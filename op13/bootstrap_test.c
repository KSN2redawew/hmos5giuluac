/* SPDX-License-Identifier: Apache-2.0
 * Tests execute pure guard/scanner functions only. No mount, BPF syscall,
 * device operation, formatter or native init is executed by this program.
 */
#define main bootstrap_entry
#include "dsu_bootstrap.c"
#undef main
#include <assert.h>

static int evaluate(const struct bpf_insn *instructions, size_t count,
                    uint32_t type, uint32_t major_number, uint32_t minor_number)
{
    uint64_t registers[11] = {0};
    uint32_t context[] = {type, major_number, minor_number};
    size_t pc = 0, steps = 0;
    while (pc < count && steps++ < 2048) {
        const struct bpf_insn *instruction = instructions + pc++;
        unsigned op = instruction->code;
        if (op == (BPF_LDX | BPF_W | BPF_MEM)) {
            assert(instruction->src_reg == 1 && instruction->off % 4 == 0);
            assert(instruction->off >= 0 && instruction->off <= 8);
            registers[instruction->dst_reg] = context[instruction->off / 4];
        } else if (op == (BPF_ALU | BPF_AND | BPF_K)) {
            registers[instruction->dst_reg] = (uint32_t)registers[instruction->dst_reg] & instruction->imm;
        } else if (op == (BPF_ALU64 | BPF_MOV | BPF_K)) {
            registers[instruction->dst_reg] = instruction->imm;
        } else if (op == (BPF_JMP | BPF_JEQ | BPF_K)) {
            if (registers[instruction->dst_reg] == (uint32_t)instruction->imm) pc += instruction->off;
        } else if (op == (BPF_JMP | BPF_JNE | BPF_K)) {
            if (registers[instruction->dst_reg] != (uint32_t)instruction->imm) pc += instruction->off;
        } else if (op == (BPF_JMP | BPF_EXIT)) {
            return registers[0];
        } else {
            assert(!"unexpected BPF instruction");
        }
    }
    assert(!"filter did not terminate");
    return -1;
}

int main(void)
{
    assert(dm_name_valid("dm-0") && dm_name_valid("dm-987"));
    assert(!dm_name_valid("dm-") && !dm_name_valid("userdata") && !dm_name_valid("dm-1/../sda"));
    devices[0] = (struct device){226, 0};
    devices[1] = (struct device){511, 7};
    device_count = 2;
    struct bpf_insn program[1024];
    size_t count = build_device_filter(program);
    for (uint32_t access = 1; access <= 7; ++access) {
        assert(evaluate(program, count, (access << 16) | 2, 226, 0) == 1);
        assert(evaluate(program, count, (access << 16) | 2, 511, 7) == 1);
        assert(evaluate(program, count, (access << 16) | 2, 511, 8) == 0);
        assert(evaluate(program, count, (access << 16) | 2, 1, 1) == 0); // /dev/mem
        assert(evaluate(program, count, (access << 16) | 2, 10, 237) == 0); // loop-control
        assert(evaluate(program, count, (access << 16) | 2, 136, 200) == 1);
        assert(evaluate(program, count, (access << 16) | 2, 143, 200) == 1);
        assert(evaluate(program, count, (access << 16) | 2, 144, 0) == 0);
        for (uint32_t major_number = 0; major_number <= 511; ++major_number) {
            assert(evaluate(program, count, (access << 16) | 1, major_number, 0) == 0);
            assert(evaluate(program, count, (access << 16) | 1, major_number, 7) == 0);
        }
    }
    char path[] = "/tmp/hmos-zero-scan-XXXXXX";
    int file = mkstemp(path);
    assert(file >= 0);
    unlink(path);
    assert(!ftruncate(file, 131073));
    assert(entirely_zero(file, 131073) == 1);
    assert(pwrite(file, "X", 1, 131072) == 1);
    assert(entirely_zero(file, 131073) == 0); // must inspect the final byte
    assert(pwrite(file, "\0", 1, 131072) == 1);
    assert(pwrite(file, "X", 1, 65536) == 1);
    assert(entirely_zero(file, 131073) == 0);
    assert(pwrite(file, "\0", 1, 65536) == 1);
    assert(entirely_zero(file, 131074) == -1 && errno == EIO); // truncated mapping
    close(file);
    puts("PASS device-filter decisions, invalid mapper names and complete zero-data scanning; kernel attachment not tested");
    return 0;
}
