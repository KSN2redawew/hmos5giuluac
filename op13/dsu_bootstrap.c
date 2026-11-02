/* SPDX-License-Identifier: Apache-2.0
 * Experimental Android first-stage -> full HarmonyOS second-stage handoff.
 * No bootloader/firmware writes or physical userdata fallback.
 * Only an exact, entirely zero-filled userdata_gsi may be initialized.
 */
#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/bpf.h>
#include <linux/capability.h>
#include <linux/fs.h>
#include <sched.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/syscall.h>
#include <sys/sysmacros.h>
#include <sys/wait.h>
#include <unistd.h>

static const char *root = "/mnt/hmos-root";
static int kmsg = -1;
static int bootlog = -1;
struct device { unsigned major, minor; };
struct binder_device_request { char name[256]; uint32_t major, minor; };
#define BINDER_DEVICE_ADD _IOWR('b', 1, struct binder_device_request)
static struct device devices[128];
static size_t device_count;

static void log_line(const char *format, ...)
{
    char message[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    dprintf(2, "hmos-op13: %s\n", message);
    if (kmsg >= 0) dprintf(kmsg, "<3>hmos-op13: %s\n", message);
    if (bootlog >= 0) dprintf(bootlog, "%s\n", message);
}

static __attribute__((noreturn)) void fatal(const char *operation)
{
    log_line("STOP %s: errno=%d (%s); no automatic reboot", operation, errno, strerror(errno));
    if (getpid() != 1) exit(1);
    for (;;) pause();
}

static int read_text(const char *path, char *buffer, size_t capacity)
{
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) return -1;
    ssize_t size = read(fd, buffer, capacity - 1);
    int saved = errno;
    close(fd);
    errno = saved;
    if (size < 0) return -1;
    buffer[size] = 0;
    while (size && (buffer[size - 1] == '\n' || buffer[size - 1] == '\r')) buffer[--size] = 0;
    return 0;
}

static void directory(const char *path, mode_t mode)
{
    if (mkdir(path, mode) && errno != EEXIST) fatal(path);
    struct stat st;
    if (lstat(path, &st) || !S_ISDIR(st.st_mode)) { errno = ENOTDIR; fatal(path); }
}

static void rootpath(char *buffer, size_t capacity, const char *suffix)
{
    if (snprintf(buffer, capacity, "%s%s", root, suffix) >= (int)capacity) {
        errno = ENAMETOOLONG; fatal(suffix);
    }
}

static void root_directory(const char *suffix, mode_t mode)
{
    char path[512];
    rootpath(path, sizeof(path), suffix);
    directory(path, mode);
}

static void bind_path(const char *source, const char *target, bool readonly)
{
    if (mount(source, target, NULL, MS_BIND, NULL)) fatal(target);
    if (readonly && mount(NULL, target, NULL, MS_BIND | MS_REMOUNT | MS_RDONLY | MS_NODEV, NULL))
        fatal(target);
}

static void root_bind(const char *source, const char *suffix, bool readonly)
{
    char target[512];
    rootpath(target, sizeof(target), suffix);
    bind_path(source, target, readonly);
}

static void root_mount(const char *source, const char *suffix, const char *type,
                       unsigned long flags, const char *options)
{
    char target[512];
    rootpath(target, sizeof(target), suffix);
    if (mount(source, target, type, flags, options)) fatal(target);
}

static void verify_dsu_mount(const char *source, const char *expected)
{
    struct stat st;
    struct statvfs fs;
    if (stat(source, &st) || !S_ISDIR(st.st_mode) || statvfs(source, &fs) || !(fs.f_flag & ST_RDONLY)) {
        errno = ENODEV; fatal("read-only DSU partition mount required");
    }
    char path[512], name[128];
    snprintf(path, sizeof(path), "/sys/dev/block/%u:%u/dm/name", major(st.st_dev), minor(st.st_dev));
    if (read_text(path, name, sizeof(name))) fatal("DSU partition mapper identity");
    if (!strcmp(name, expected)) return;
    // Android may put a dm-verity device above the exact *_gsi mapping.
    // Accept one extra DM layer only when its sole slave is that mapping.
    snprintf(path, sizeof(path), "/sys/dev/block/%u:%u/slaves", major(st.st_dev), minor(st.st_dev));
    DIR *slaves = opendir(path);
    if (!slaves) fatal("DSU verified mapper slaves");
    struct dirent *entry;
    unsigned count = 0, matching = 0;
    while ((entry = readdir(slaves))) {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) continue;
        ++count;
        snprintf(path, sizeof(path), "/sys/class/block/%.128s/dm/name", entry->d_name);
        if (!read_text(path, name, sizeof(name)) && !strcmp(name, expected)) ++matching;
    }
    closedir(slaves);
    if (count != 1 || matching != 1) { errno = ENODEV; fatal("partition is not backed by expected DSU mapping"); }
}

static void dsu_bind(const char *source, const char *target, const char *expected)
{
    verify_dsu_mount(source, expected);
    root_bind(source, target, true);
    log_line("bound separate DSU partition %s from %s", expected, source);
}

static bool dm_name_valid(const char *name)
{
    if (strncmp(name, "dm-", 3) || !name[3]) return false;
    for (const char *p = name + 3; *p; ++p) if (*p < '0' || *p > '9') return false;
    return true;
}

static dev_t find_dsu_data(void)
{
    DIR *dir = opendir("/sys/block");
    if (!dir) fatal("sys/block");
    struct dirent *entry;
    bool system_found = false;
    dev_t data = 0;
    while ((entry = readdir(dir))) {
        if (!dm_name_valid(entry->d_name)) continue;
        char path[512], name[128], numbers[64];
        snprintf(path, sizeof(path), "/sys/block/%.64s/dm/name", entry->d_name);
        if (read_text(path, name, sizeof(name))) continue;
        if (!strcmp(name, "system_gsi")) system_found = true;
        if (strcmp(name, "userdata_gsi")) continue;
        snprintf(path, sizeof(path), "/sys/block/%.64s/dev", entry->d_name);
        unsigned major_number, minor_number;
        if (read_text(path, numbers, sizeof(numbers)) || sscanf(numbers, "%u:%u", &major_number, &minor_number) != 2)
            fatal("userdata_gsi device identity");
        if (data) { errno = EEXIST; fatal("multiple userdata_gsi mappings"); }
        data = makedev(major_number, minor_number);
    }
    closedir(dir);
    if (!system_found || !data) { errno = ENODEV; fatal("exact system_gsi/userdata_gsi required"); }
    return data;
}

static void load_stock_modules(void)
{
    FILE *list = fopen("/hmos/modules/load-order", "re");
    if (!list) fatal("stock module order");
    char name[128];
    while (fgets(name, sizeof(name), list)) {
        name[strcspn(name, "\r\n")] = 0;
        if (!name[0]) continue;
        for (const char *p = name; *p; ++p)
            if (!((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') ||
                  (*p >= '0' && *p <= '9') || *p == '_')) { errno = EINVAL; fatal("module name"); }
        char path[256];
        snprintf(path, sizeof(path), "/sys/module/%s", name);
        if (!access(path, F_OK)) continue;
        snprintf(path, sizeof(path), "/hmos/modules/%s.ko", name);
        int fd = open(path, O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
        if (fd < 0) fatal(path);
        int result = syscall(__NR_finit_module, fd, "", 0); // no force flags
        int saved = errno;
        close(fd);
        errno = saved;
        if (result && errno != EEXIST) fatal(name);
        log_line("loaded stock module %s", name);
    }
    fclose(list);
}

static void copy_device(const char *suffix, bool required)
{
    char original[256], target[512];
    snprintf(original, sizeof(original), "/dev/%s", suffix);
    struct stat st;
    if (stat(original, &st)) {
        char sysfile[256], numbers[64];
        const char *name = strrchr(suffix, '/');
        name = name ? name + 1 : suffix;
        const char *class_name = !strncmp(suffix, "dri/", 4) ? "drm" :
            !strncmp(suffix, "dma_heap/", 9) ? "dma_heap" :
            !strncmp(suffix, "input/", 6) ? "input" :
            !strcmp(suffix, "kgsl-3d0") ? "kgsl" : "misc";
        snprintf(sysfile, sizeof(sysfile), "/sys/class/%s/%s/dev", class_name, name);
        unsigned major_number = 0, minor_number = 0;
        if (read_text(sysfile, numbers, sizeof(numbers)) || sscanf(numbers, "%u:%u", &major_number, &minor_number) != 2) {
            if (required) fatal(sysfile);
            return;
        }
        memset(&st, 0, sizeof(st));
        st.st_mode = S_IFCHR;
        st.st_rdev = makedev(major_number, minor_number);
    }
    if (!S_ISCHR(st.st_mode) || device_count == 128) { errno = EINVAL; fatal(original); }
    char relative[256];
    snprintf(relative, sizeof(relative), "/dev/%s", suffix);
    rootpath(target, sizeof(target), relative);
    if (mknod(target, S_IFCHR | 0666, st.st_rdev)) fatal(target);
    if (!strncmp(suffix, "dri/", 4)) {
        if (chown(target, 0, 1003) || chmod(target, 0660)) fatal("DRM device ownership");
    }
    devices[device_count++] = (struct device){major(st.st_rdev), minor(st.st_rdev)};
}

static void binder_namespace(void)
{
    root_directory("/dev/hmos-binderfs", 0700);
    root_mount("binder", "/dev/hmos-binderfs", "binder", MS_NOSUID | MS_NOEXEC, NULL);
    char control_path[512];
    rootpath(control_path, sizeof(control_path), "/dev/hmos-binderfs/binder-control");
    int control = open(control_path, O_RDWR | O_CLOEXEC);
    if (control < 0) fatal("private BinderFS control");
    const char *names[] = {"binder", "hwbinder", "vndbinder"};
    for (size_t i = 0; i < 3; ++i) {
        struct binder_device_request request = {0};
        snprintf(request.name, sizeof(request.name), "hmos-%s", names[i]);
        if (ioctl(control, BINDER_DEVICE_ADD, &request)) fatal("private BinderFS device");
        char source[512], suffix[256], alias[512];
        snprintf(suffix, sizeof(suffix), "/dev/hmos-binderfs/%s", request.name);
        rootpath(source, sizeof(source), suffix);
        snprintf(suffix, sizeof(suffix), "/dev/%s", names[i]);
        rootpath(alias, sizeof(alias), suffix);
        if (chmod(source, 0666)) fatal("BinderFS permissions");
        int placeholder = open(alias, O_CREAT | O_EXCL | O_WRONLY | O_CLOEXEC, 0600);
        if (placeholder < 0) fatal(alias);
        close(placeholder);
        // A plain mknod of a BinderFS major/minor loses inode-private driver
        // data. Bind the actual BinderFS inode instead.
        bind_path(source, alias, false);
        if (device_count == 128) { errno = ENOSPC; fatal("device allowlist"); }
        devices[device_count++] = (struct device){request.major, request.minor};
    }
    close(control);
    // Guest applications only see the three native aliases. Keep control
    // inaccessible; the alias bind mounts hold their real BinderFS inodes.
    root_mount("tmpfs", "/dev/hmos-binderfs", "tmpfs", MS_RDONLY | MS_NOSUID | MS_NODEV | MS_NOEXEC, "mode=0000");
}

static void device_namespace(void)
{
    binder_namespace();
    const char *base[] = {"null", "zero", "full", "random", "urandom", "tty", "kmsg",
                         "ptmx", "ashmem", "fuse", "sw_sync", "kgsl-3d0"};
    for (size_t i = 0; i < sizeof(base) / sizeof(base[0]); ++i)
        copy_device(base[i], !strcmp(base[i], "kgsl-3d0"));
    root_directory("/dev/dri", 0755);
    root_directory("/dev/dma_heap", 0755);
    root_directory("/dev/input", 0755);
    copy_device("dri/card0", true);
    copy_device("dri/renderD128", false);
    copy_device("dma_heap/system", true);
    copy_device("dma_heap/qcom,system", false);
    copy_device("dma_heap/qcom,display", false);
    DIR *inputs = opendir("/dev/input");
    struct dirent *entry;
    while (inputs && (entry = readdir(inputs))) {
        if (strncmp(entry->d_name, "event", 5)) continue;
        char relative[128];
        snprintf(relative, sizeof(relative), "input/%.64s", entry->d_name);
        copy_device(relative, false);
    }
    if (inputs) closedir(inputs);
    // Newly probed input modules may have sysfs entries before ueventd has
    // ever created Android /dev nodes. Enumerate those entries as well.
    inputs = opendir("/sys/class/input");
    while (inputs && (entry = readdir(inputs))) {
        if (strncmp(entry->d_name, "event", 5)) continue;
        char relative[128], guest[512], suffix[256];
        snprintf(relative, sizeof(relative), "input/%.64s", entry->d_name);
        snprintf(suffix, sizeof(suffix), "/dev/%s", relative);
        rootpath(guest, sizeof(guest), suffix);
        if (access(guest, F_OK)) copy_device(relative, false);
    }
    if (inputs) closedir(inputs);
    root_directory("/dev/pts", 0755);
    root_directory("/dev/shm", 01777);
    root_directory("/dev/socket", 0755);
    root_directory("/dev/unix/socket", 0755); // parent created by caller
    root_mount("devpts", "/dev/pts", "devpts", MS_NOSUID | MS_NOEXEC, "newinstance,mode=0620,ptmxmode=0666");
}

#define INS(code_, dst_, src_, off_, imm_) ((struct bpf_insn){.code=(code_),.dst_reg=(dst_),.src_reg=(src_),.off=(off_),.imm=(imm_)})
static size_t build_device_filter(struct bpf_insn program[1024])
{
    size_t n = 0;
    program[n++] = INS(BPF_LDX | BPF_W | BPF_MEM, 2, 1, 0, 0);
    program[n++] = INS(BPF_ALU | BPF_AND | BPF_K, 2, 0, 0, 0xffff);
    program[n++] = INS(BPF_JMP | BPF_JEQ | BPF_K, 2, 0, 2, 2); // char only
    program[n++] = INS(BPF_ALU64 | BPF_MOV | BPF_K, 0, 0, 0, 0);
    program[n++] = INS(BPF_JMP | BPF_EXIT, 0, 0, 0, 0);
    program[n++] = INS(BPF_LDX | BPF_W | BPF_MEM, 2, 1, 4, 0); // major
    program[n++] = INS(BPF_LDX | BPF_W | BPF_MEM, 3, 1, 8, 0); // minor
    for (size_t i = 0; i < device_count; ++i) {
        program[n++] = INS(BPF_JMP | BPF_JNE | BPF_K, 2, 0, 3, devices[i].major);
        program[n++] = INS(BPF_JMP | BPF_JNE | BPF_K, 3, 0, 2, devices[i].minor);
        program[n++] = INS(BPF_ALU64 | BPF_MOV | BPF_K, 0, 0, 0, 1);
        program[n++] = INS(BPF_JMP | BPF_EXIT, 0, 0, 0, 0);
    }
    // PTYs created after devpts is mounted (Linux UNIX98 majors 136..143).
    for (unsigned major_number = 136; major_number <= 143; ++major_number) {
        program[n++] = INS(BPF_JMP | BPF_JNE | BPF_K, 2, 0, 2, major_number);
        program[n++] = INS(BPF_ALU64 | BPF_MOV | BPF_K, 0, 0, 0, 1);
        program[n++] = INS(BPF_JMP | BPF_EXIT, 0, 0, 0, 0);
    }
    program[n++] = INS(BPF_ALU64 | BPF_MOV | BPF_K, 0, 0, 0, 0);
    program[n++] = INS(BPF_JMP | BPF_EXIT, 0, 0, 0, 0);
    return n;
}

static void isolate_devices(void)
{
    // Attach above a child cgroup, then hide that ancestor in a cgroup namespace.
    // This keeps the filter effective if native init creates delegated children.
    directory("/mnt/hmos-cgroup", 0700);
    if (mount("cgroup2", "/mnt/hmos-cgroup", "cgroup2", MS_NOSUID | MS_NODEV | MS_NOEXEC, NULL))
        fatal("private cgroup2 mount");
    directory("/mnt/hmos-cgroup/hmos", 0700);
    directory("/mnt/hmos-cgroup/hmos/payload", 0700);
    int group = open("/mnt/hmos-cgroup/hmos", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (group < 0) fatal("device cgroup");
    struct bpf_insn program[1024];
    size_t n = build_device_filter(program);
    char verifier[16384] = {0};
    union bpf_attr load = {0};
    load.prog_type = BPF_PROG_TYPE_CGROUP_DEVICE;
    load.insn_cnt = n;
    load.insns = (uint64_t)(uintptr_t)program;
    load.license = (uint64_t)(uintptr_t)"Apache";
    load.log_buf = (uint64_t)(uintptr_t)verifier;
    load.log_size = sizeof(verifier);
    load.log_level = 1;
    int fd = syscall(__NR_bpf, BPF_PROG_LOAD, &load, sizeof(load));
    if (fd < 0) { log_line("BPF verifier: %s", verifier); fatal("device filter load"); }
    union bpf_attr attach = {0};
    attach.target_fd = group;
    attach.attach_bpf_fd = fd;
    attach.attach_type = BPF_CGROUP_DEVICE;
    if (syscall(__NR_bpf, BPF_PROG_ATTACH, &attach, sizeof(attach))) fatal("device filter attach");
    close(fd);
    close(group);
    int membership = open("/mnt/hmos-cgroup/hmos/payload/cgroup.procs", O_WRONLY | O_CLOEXEC);
    if (membership < 0 || write(membership, "1\n", 2) != 2) fatal("device cgroup membership");
    close(membership);
    if (unshare(CLONE_NEWCGROUP)) fatal("cgroup namespace");
    log_line("device filter attached: block devices denied; %zu character nodes allowed", device_count);
}

static void root_link(const char *target, const char *suffix)
{
    char path[512];
    rootpath(path, sizeof(path), suffix);
    if (symlink(target, path)) fatal(path);
}

static void mask_extra_init(void)
{
    FILE *list = fopen("/hmos/mask-init-dirs", "re");
    if (!list) fatal("init mask manifest");
    char suffix[384];
    while (fgets(suffix, sizeof(suffix), list)) {
        suffix[strcspn(suffix, "\r\n")] = 0;
        if (!suffix[0]) continue;
        bool prefix = !strncmp(suffix, "/sys_prod/", 10) || !strncmp(suffix, "/chip_prod/", 11) ||
            !strncmp(suffix, "/cust/", 6) || !strncmp(suffix, "/patch_hw/", 10);
        if (!prefix || strstr(suffix, "..") || !strstr(suffix, "/etc/init")) {
            errno = EINVAL; fatal("invalid init mask path");
        }
        root_bind("/hmos/empty", suffix, true);
    }
    fclose(list);
}

static void verify_data_node(int fd, dev_t expected)
{
    struct stat st;
    char path[256], name[128];
    if (fstat(fd, &st) || !S_ISBLK(st.st_mode) || st.st_rdev != expected) {
        errno = ENODEV; fatal("DSU data descriptor identity");
    }
    snprintf(path, sizeof(path), "/sys/dev/block/%u:%u/dm/name", major(expected), minor(expected));
    if (read_text(path, name, sizeof(name)) || strcmp(name, "userdata_gsi")) {
        errno = ENODEV; fatal("DSU data mapping changed");
    }
}

static int entirely_zero(int fd, uint64_t bytes)
{
    unsigned char buffer[65536];
    for (uint64_t offset = 0; offset < bytes;) {
        size_t wanted = bytes - offset < sizeof(buffer) ? bytes - offset : sizeof(buffer);
        ssize_t got = pread(fd, buffer, wanted, offset);
        if (got <= 0) { if (!got) errno = EIO; return -1; }
        for (ssize_t i = 0; i < got; ++i) if (buffer[i]) return 0;
        offset += got;
    }
    return 1;
}

static void initialize_blank_dsu_data(const char *node, dev_t expected)
{
    int fd = open(node, O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
    if (fd < 0) fatal("DSU data descriptor");
    verify_data_node(fd, expected);
    uint64_t bytes = 0;
    if (ioctl(fd, BLKGETSIZE64, &bytes) || bytes < 128ULL * 1024 * 1024 ||
        bytes > 512ULL * 1024 * 1024 * 1024) { errno = EINVAL; fatal("DSU data size"); }
    // A failed mount is never sufficient reason to format. Scan the entire
    // exact mapping and refuse if even one nonzero byte exists anywhere.
    int blank = entirely_zero(fd, bytes);
    if (blank < 0) fatal("blank DSU data scan");
    if (!blank) { errno = EEXIST; fatal("unmountable nonblank DSU data preserved"); }
    verify_data_node(fd, expected);
    log_line("entire userdata_gsi mapping is zero-filled; initializing isolated ext4");
    pid_t child = fork();
    if (child < 0) fatal("DSU formatter fork");
    if (!child) {
        if (syscall(__NR_close_range, 3U, ~0U, 0) || chdir(root) || chroot(".") || chdir("/")) _exit(125);
        char *const args[] = {"/system/bin/mke2fs", "-t", "ext4", "-b", "4096", "-I", "256", "-O", "encrypt,verity", "-F", "-m", "0",
                              "-L", "hmos-dsu", "/dev/block/userdata_gsi", NULL};
        char *const environment[] = {"PATH=/system/bin:/vendor/bin", NULL};
        execve(args[0], args, environment);
        _exit(126);
    }
    int status;
    if (waitpid(child, &status, 0) != child || !WIFEXITED(status) || WEXITSTATUS(status)) {
        errno = EIO; fatal("isolated DSU formatter");
    }
    verify_data_node(fd, expected);
    close(fd);
}

int main(int argc, char **argv)
{
    if (getpid() != 1 || argc != 2 || strcmp(argv[1], "selinux_setup")) {
        fputs("This executable is only for Android DSU PID-1 handoff. No action taken.\n", stderr);
        return 2;
    }
    kmsg = open("/dev/kmsg", O_WRONLY | O_CLOEXEC);
    umask(0);
    log_line("starting experimental stock-kernel DSU handoff");
    dev_t data = find_dsu_data();
    if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL)) fatal("private mount propagation");
    directory(root, 0755);
    if (mount("tmpfs", root, "tmpfs", MS_NOSUID, "mode=0755")) fatal("new root");
    const char *dirs[] = {"/system", "/vendor", "/odm",
        "/chip_prod", "/sys_prod", "/cust", "/patch_hw", "/data", "/dev", "/proc", "/sys",
        "/mnt", "/tmp", "/log", "/config", "/storage", "/sec_storage", "/module_update",
        "/preload", "/version", "/updater", "/eng_chipset", "/eng_system"};
    for (size_t i = 0; i < sizeof(dirs) / sizeof(dirs[0]); ++i) root_directory(dirs[i], 0755);
    dsu_bind("/system", "/system", "system_gsi");
    root_bind("/system/bin/hmos-native-init", "/system/bin/init", true);
    dsu_bind("/vendor", "/vendor", "vendor_gsi");
    dsu_bind("/odm", "/odm", "odm_gsi");
    char target[512];
    dsu_bind("/chip_prod", "/chip_prod", "chip_prod_gsi");
    dsu_bind("/sys_prod", "/sys_prod", "sys_prod_gsi");
    dsu_bind("/cust", "/cust", "cust_gsi");
    dsu_bind("/patch_hw", "/patch_hw", "patch_hw_gsi");
    root_link("system/bin", "/bin");
    root_link("system/etc", "/etc");
    root_link("system/lib", "/lib");
    root_link("system/lib64", "/lib64");
    root_link("system/bin/init", "/init");
    root_link("vendor", "/chipset");
    root_mount("tmpfs", "/dev", "tmpfs", MS_NOSUID, "mode=0755");
    root_directory("/dev/block", 0700);
    root_directory("/dev/unix", 0755);
    // Mount only the exact DSU identity. Existing nonblank data is never
    // formatted, repaired, or replaced with physical Android storage.
    rootpath(target, sizeof(target), "/dev/block/userdata_gsi");
    if (mknod(target, S_IFBLK | 0600, data)) fatal("DSU data node");
    root_mount("proc", "/proc", "proc", MS_NOSUID | MS_NODEV | MS_NOEXEC, NULL);
    root_mount("sysfs", "/sys", "sysfs", MS_RDONLY | MS_NOSUID | MS_NODEV | MS_NOEXEC, NULL);
    char data_target[512];
    rootpath(data_target, sizeof(data_target), "/data");
    int data_fd = open(target, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (data_fd < 0) fatal("pin DSU data descriptor");
    verify_data_node(data_fd, data);
    char pinned_data[64];
    snprintf(pinned_data, sizeof(pinned_data), "/proc/self/fd/%d", data_fd);
    if (mount(pinned_data, data_target, "ext4", MS_NOSUID | MS_NODEV, NULL) &&
        mount(pinned_data, data_target, "f2fs", MS_NOSUID | MS_NODEV, NULL)) {
        initialize_blank_dsu_data(target, data);
        verify_data_node(data_fd, data);
        if (mount(pinned_data, data_target, "ext4", MS_NOSUID | MS_NODEV, NULL)) fatal("initialized DSU data mount");
    }
    close(data_fd);
    if (unlink(target)) fatal("hide DSU block node");
    rootpath(target, sizeof(target), "/data/hmos-port");
    directory(target, 0700);
    rootpath(target, sizeof(target), "/data/hmos-port/bootstrap.log");
    bootlog = open(target, O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0600);
    if (bootlog < 0) fatal("persistent bootstrap log");
    log_line("persistent data mounted exclusively from userdata_gsi");
    load_stock_modules();
    device_namespace();
    const char *brightness = "/sys/class/backlight/panel0-backlight/brightness";
    if (!access(brightness, F_OK)) {
        if (chown(brightness, 0, 1003) || chmod(brightness, 0660)) fatal("backlight ownership");
        root_bind(brightness, brightness, false);
    }
    // Curated startup files mask every auto-scanned vendor/chipset init tree.
    root_bind("/hmos/startup/init.cfg", "/system/etc/init.cfg", true);
    root_bind("/hmos/startup/system-init", "/system/etc/init", true);
    root_bind("/hmos/init-modules", "/system/lib64/init", true);
    root_bind("/hmos/startup/vendor-init", "/vendor/etc/init", true);
    mask_extra_init();
    root_bind("/hmos/startup/hdf_default.hcb", "/chip_prod/etc/hdfconfig/hdf_default.hcb", true);
    root_bind("/hmos/policy/policy.31", "/vendor/etc/selinux/prebuild_sepolicy/policy.31", true);
    root_bind("/hmos/policy/developer_policy", "/vendor/etc/selinux/prebuild_sepolicy/developer_policy", true);
    root_bind("/hmos/display/libdisplay_buffer_vdi_impl.z.so", "/vendor/lib64/libdisplay_buffer_vdi_impl.z.so", true);
    root_bind("/hmos/display/libdisplay_composer_vdi_impl.z.so", "/vendor/lib64/libdisplay_composer_vdi_impl.z.so", true);
    root_bind("/hmos/ui/display_manager_config.xml", "/sys_prod/etc/window/resources/display_manager_config.xml", true);
    root_bind("/hmos/ui/hgm_policy_config.xml", "/sys_prod/etc/graphic/hgm_policy_config.xml", true);
    isolate_devices();
    // No automatic critical-service reboot or raw-I/O/module privilege after
    // the curated stock module load. Native storage still retains SYS_ADMIN.
    if (prctl(PR_CAPBSET_DROP, CAP_SYS_BOOT, 0, 0, 0) ||
        prctl(PR_CAPBSET_DROP, CAP_SYS_RAWIO, 0, 0, 0) ||
        prctl(PR_CAPBSET_DROP, CAP_SYS_MODULE, 0, 0, 0)) fatal("capability bounding set");
    root_directory("/sys/fs/selinux", 0755);
    // Expose only delegated cgroup children. The device-filter ancestor is
    // outside this cgroup namespace and outside the guest's mount tree.
    root_directory("/dev/cgroup", 0755);
    root_mount("cgroup2", "/dev/cgroup", "cgroup2", MS_NOSUID | MS_NODEV | MS_NOEXEC, NULL);
    log_line("executing full donor init --second-stage; framework/SceneBoard target");
    if (chdir(root) || chroot(".") || chdir("/")) fatal("chroot");
    // Include inherited first-stage fds in the close, not only our own fds.
    if (syscall(__NR_close_range, 3U, ~0U, 0)) fatal("close inherited descriptors");
    kmsg = bootlog = -1;
    int null = open("/dev/null", O_RDWR);
    if (null < 0) fatal("guest null device");
    for (int fd = 0; fd < 3; ++fd) if (dup2(null, fd) < 0) fatal("guest stdio");
    if (null > 2) close(null);
    // No old-root directory fd or block fd is inherited by the native graph.
    char *const init_args[] = {"/system/bin/init", "--second-stage", "0", NULL};
    char *const environment[] = {"PATH=/system/bin:/vendor/bin", "OHOS_MESA_SOFTWARE=true", NULL};
    execve(init_args[0], init_args, environment);
    kmsg = open("/dev/kmsg", O_WRONLY | O_CLOEXEC);
    fatal("native init exec");
}
