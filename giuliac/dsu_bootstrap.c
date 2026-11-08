/* SPDX-License-Identifier: Apache-2.0
 * Experimental Android first-stage -> full HarmonyOS second-stage handoff for OnePlus Ace 5 (giuliac).
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
#include <sys/vfs.h>
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
    dprintf(2, "hmos-giuliac: %s\n", message);
    if (kmsg >= 0) dprintf(kmsg, "<3>hmos-giuliac: %s\n", message);
    if (bootlog >= 0) dprintf(bootlog, "%s\n", message);
    int meta_fd = open("/metadata/hmos-bootstrap.log", O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0666);
    if (meta_fd >= 0) {
        dprintf(meta_fd, "%s\n", message);
        fdatasync(meta_fd);
        close(meta_fd);
    }
}

static __attribute__((noreturn)) void fatal(const char *operation)
{
    log_line("STOP %s: errno=%d (%s); no automatic reboot", operation, errno, strerror(errno));
    sync();
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

static bool dm_name_valid(const char *name)
{
    if (strncmp(name, "dm-", 3) || !name[3]) return false;
    for (const char *p = name + 3; *p; ++p) if (*p < '0' || *p > '9') return false;
    return true;
}

static dev_t find_dsu_dev(const char *expected_name)
{
    DIR *dir = opendir("/sys/block");
    if (!dir) return 0;
    struct dirent *entry;
    dev_t res = 0;
    while ((entry = readdir(dir))) {
        if (!dm_name_valid(entry->d_name)) continue;
        char path[512], name[128], numbers[64];
        snprintf(path, sizeof(path), "/sys/block/%.64s/dm/name", entry->d_name);
        if (read_text(path, name, sizeof(name))) continue;
        bool match = !strcmp(name, expected_name);
        if (!match) {
            snprintf(path, sizeof(path), "/sys/block/%.64s/slaves", entry->d_name);
            DIR *slaves = opendir(path);
            if (slaves) {
                struct dirent *se;
                while ((se = readdir(slaves))) {
                    if (!strcmp(se->d_name, ".") || !strcmp(se->d_name, "..")) continue;
                    char spath[512], sname[128];
                    snprintf(spath, sizeof(spath), "/sys/class/block/%.128s/dm/name", se->d_name);
                    if (!read_text(spath, sname, sizeof(sname)) && !strcmp(sname, expected_name)) {
                        match = true;
                        break;
                    }
                }
                closedir(slaves);
            }
        }
        if (match) {
            snprintf(path, sizeof(path), "/sys/block/%.64s/dev", entry->d_name);
            unsigned major_num, minor_num;
            if (!read_text(path, numbers, sizeof(numbers)) && sscanf(numbers, "%u:%u", &major_num, &minor_num) == 2) {
                res = makedev(major_num, minor_num);
                break;
            }
        }
    }
    closedir(dir);
    return res;
}



static void mount_or_bind_dsu(const char *mountpoint, const char *dm_name, bool required)
{
    char target[512];
    rootpath(target, sizeof(target), mountpoint);

    struct stat st;
    if (!stat(mountpoint, &st) && S_ISDIR(st.st_mode)) {
        struct statfs sfs;
        if (!statfs(mountpoint, &sfs) && (unsigned long)sfs.f_type != 0x01021994UL /* TMPFS */ &&
            (unsigned long)sfs.f_type != 0x858458f6UL /* RAMFS */) {
            root_bind(mountpoint, mountpoint, true);
            log_line("bound existing %s mount into new root (%s)", mountpoint, dm_name);
            return;
        }
    }

    dev_t dev = find_dsu_dev(dm_name);
    if (dev) {
        char nodepath[512];
        snprintf(nodepath, sizeof(nodepath), "/dev/block_%s", dm_name);
        unlink(nodepath);
        if (mknod(nodepath, S_IFBLK | 0600, dev) == 0) {
            int res = mount(nodepath, target, "erofs", MS_RDONLY | MS_NODEV | MS_NOATIME, NULL);
            if (res != 0) res = mount(nodepath, target, "ext4", MS_RDONLY | MS_NODEV | MS_NOATIME, NULL);
            unlink(nodepath);
            if (res == 0) {
                log_line("mounted separate DSU partition %s (%u:%u) to %s", dm_name, major(dev), minor(dev), mountpoint);
                return;
            } else {
                log_line("mount failed for DSU partition %s: errno=%d (%s)", dm_name, errno, strerror(errno));
            }
        } else {
            log_line("mknod failed for DSU partition %s: errno=%d (%s)", dm_name, errno, strerror(errno));
        }
    } else {
        log_line("block device for DSU partition %s not found in /sys/block", dm_name);
    }

    if (required) {
        errno = ENODEV;
        fatal("required DSU partition mount failed");
    } else {
        log_line("warning: non-critical DSU partition %s skipped", dm_name);
    }
}

static dev_t find_dsu_data(void)
{
    dev_t data = find_dsu_dev("userdata_gsi");
    if (!data) {
        DIR *dir = opendir("/sys/block");
        if (dir) {
            struct dirent *entry;
            while ((entry = readdir(dir))) {
                if (!dm_name_valid(entry->d_name)) continue;
                char path[512], name[128];
                snprintf(path, sizeof(path), "/sys/block/%.64s/dm/name", entry->d_name);
                if (read_text(path, name, sizeof(name))) continue;
                if (strstr(name, "userdata")) {
                    data = find_dsu_dev(name);
                    if (data) break;
                }
            }
            closedir(dir);
        }
    }
    if (!data) {
        log_line("STOP: userdata_gsi block device not found in /sys/block");
        errno = ENODEV; fatal("find userdata_gsi");
    }
    log_line("located userdata_gsi device (%u:%u)", major(data), minor(data));
    return data;
}

static void load_stock_modules(void)
{
    const char *module_lists[] = {
        "/vendor_dlkm/lib/modules/modules.load",
        "/system_dlkm/lib/modules/modules.load",
        "/hmos/modules/load-order"
    };
    for (size_t l = 0; l < sizeof(module_lists) / sizeof(module_lists[0]); ++l) {
        FILE *list = fopen(module_lists[l], "re");
        if (!list) continue;
        log_line("loading kernel modules from %s...", module_lists[l]);
        char line[640];
        while (fgets(line, sizeof(line), list)) {
            line[strcspn(line, "\r\n")] = 0;
            if (!line[0] || line[0] == '#') continue;
            char mod_name[128] = {0}, mod_path[384] = {0};
            if (sscanf(line, "%127s %383s", mod_name, mod_path) == 2) {
                // already has explicit path
            } else if (sscanf(line, "%127s", mod_name) == 1) {
                if (strstr(module_lists[l], "vendor_dlkm")) {
                    snprintf(mod_path, sizeof(mod_path), "/vendor_dlkm/lib/modules/%s", mod_name);
                } else {
                    snprintf(mod_path, sizeof(mod_path), "/system_dlkm/lib/modules/%s", mod_name);
                }
            } else {
                continue;
            }
            int fd = open(mod_path, O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
            if (fd >= 0) {
                int result = syscall(__NR_finit_module, fd, "", 0);
                close(fd);
                if (result == 0) {
                    log_line("loaded stock module %s", mod_name);
                }
            }
        }
        fclose(list);
    }
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
            log_line("device %s not found in sysfs (%s); skipping", suffix, sysfile);
            if (required) fatal(sysfile);
            return;
        }
        memset(&st, 0, sizeof(st));
        st.st_mode = S_IFCHR;
        st.st_rdev = makedev(major_number, minor_number);
    }
    if (!S_ISCHR(st.st_mode) || device_count == 128) {
        if (required) { errno = EINVAL; fatal(original); }
        return;
    }
    char relative[256];
    snprintf(relative, sizeof(relative), "/dev/%s", suffix);
    rootpath(target, sizeof(target), relative);
    if (mknod(target, S_IFCHR | 0666, st.st_rdev)) {
        if (errno != EEXIST) fatal(target);
    }
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
        if (placeholder < 0 && errno != EEXIST) fatal(alias);
        if (placeholder >= 0) close(placeholder);
        bind_path(source, alias, false);
        if (device_count == 128) { errno = ENOSPC; fatal("device allowlist"); }
        devices[device_count++] = (struct device){request.major, request.minor};
    }
    close(control);
    root_mount("tmpfs", "/dev/hmos-binderfs", "tmpfs", MS_RDONLY | MS_NOSUID | MS_NODEV | MS_NOEXEC, "mode=0000");
}

static void device_namespace(void)
{
    binder_namespace();
    const char *base[] = {"null", "zero", "full", "random", "urandom", "tty", "kmsg",
                         "ptmx", "ashmem", "fuse", "sw_sync", "kgsl-3d0"};
    for (size_t i = 0; i < sizeof(base) / sizeof(base[0]); ++i)
        copy_device(base[i], false);
    root_directory("/dev/dri", 0755);
    root_directory("/dev/dma_heap", 0755);
    root_directory("/dev/input", 0755);
    copy_device("dri/card0", false);
    copy_device("dri/renderD128", false);
    copy_device("dma_heap/system", false);
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
    root_directory("/dev/unix/socket", 0755);
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
    directory("/mnt/hmos-cgroup", 0700);
    if (mount("cgroup2", "/mnt/hmos-cgroup", "cgroup2", MS_NOSUID | MS_NODEV | MS_NOEXEC, NULL)) {
        log_line("cgroup2 mount skipped or already mounted");
        return;
    }
    directory("/mnt/hmos-cgroup/hmos", 0700);
    directory("/mnt/hmos-cgroup/hmos/payload", 0700);
    int group = open("/mnt/hmos-cgroup/hmos", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (group < 0) return;
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
    if (fd >= 0) {
        union bpf_attr attach = {0};
        attach.target_fd = group;
        attach.attach_bpf_fd = fd;
        attach.attach_type = BPF_CGROUP_DEVICE;
        syscall(__NR_bpf, BPF_PROG_ATTACH, &attach, sizeof(attach));
        close(fd);
    }
    close(group);
    int membership = open("/mnt/hmos-cgroup/hmos/payload/cgroup.procs", O_WRONLY | O_CLOEXEC);
    if (membership >= 0) {
        write(membership, "1\n", 2);
        close(membership);
    }
    unshare(CLONE_NEWCGROUP);
    log_line("device filter attached: %zu character nodes allowed", device_count);
}

static void root_link(const char *target, const char *suffix)
{
    char path[512];
    rootpath(path, sizeof(path), suffix);
    if (symlink(target, path) && errno != EEXIST) fatal(path);
}

static void mask_extra_init(void)
{
    FILE *list = fopen("/hmos/mask-init-dirs", "re");
    if (!list) return;
    char suffix[384];
    while (fgets(suffix, sizeof(suffix), list)) {
        suffix[strcspn(suffix, "\r\n")] = 0;
        if (!suffix[0]) continue;
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

static void make_basic_dev_nodes(void)
{
    char p[512];
    rootpath(p, sizeof(p), "/dev/null");
    mknod(p, S_IFCHR | 0666, makedev(1, 3));
    rootpath(p, sizeof(p), "/dev/zero");
    mknod(p, S_IFCHR | 0666, makedev(1, 5));
    rootpath(p, sizeof(p), "/dev/random");
    mknod(p, S_IFCHR | 0666, makedev(1, 8));
    rootpath(p, sizeof(p), "/dev/urandom");
    mknod(p, S_IFCHR | 0666, makedev(1, 9));
}

static void initialize_blank_dsu_data(const char *node, dev_t expected)
{
    int fd = open(node, O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
    if (fd < 0) fatal("DSU data descriptor");
    verify_data_node(fd, expected);
    uint64_t bytes = 0;
    if (ioctl(fd, BLKGETSIZE64, &bytes) || bytes < 128ULL * 1024 * 1024) {
        bytes = 0;
    }
    verify_data_node(fd, expected);
    close(fd);

    log_line("userdata_gsi unformatted/corrupt (size %llu MB); initializing ext4 filesystem",
             (unsigned long long)(bytes / (1024 * 1024)));

    make_basic_dev_nodes();

    pid_t child = fork();
    if (child < 0) fatal("DSU formatter fork");
    if (!child) {
        if (syscall(__NR_close_range, 3U, ~0U, 0) || chdir(root) || chroot(".") || chdir("/")) _exit(125);
        char *const args[] = {"/system/bin/mke2fs", "-t", "ext4", "-b", "4096", "-F", "-m", "0",
                              "-L", "hmos-dsu", "/dev/block/userdata_gsi", NULL};
        char *const environment[] = {
            "PATH=/system/bin:/vendor/bin",
            "LD_LIBRARY_PATH=/system/lib64:/system/lib64/platformsdk:/system/lib64/chipset-pub-sdk:/system/lib64/ndk",
            NULL
        };
        execve(args[0], args, environment);
        _exit(126);
    }
    int status = 0;
    if (waitpid(child, &status, 0) != child || !WIFEXITED(status) || WEXITSTATUS(status)) {
        log_line("mke2fs exited with status 0x%x; errno=%d", status, errno);
        errno = EIO; fatal("isolated DSU formatter");
    }
    log_line("mke2fs formatted userdata_gsi successfully");
}

int main(int argc, char **argv)
{
    if (getpid() != 1) {
        fputs("This executable is only for Android DSU PID-1 handoff. No action taken.\n", stderr);
        return 2;
    }
    kmsg = open("/dev/kmsg", O_WRONLY | O_CLOEXEC);
    umask(0);
    log_line("starting experimental stock-kernel DSU handoff for OnePlus Ace 5 (giuliac), argc=%d argv[1]=%s",
             argc, argc > 1 ? argv[1] : "(null)");
    dev_t data = find_dsu_data();
    if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL)) fatal("private mount propagation");
    directory(root, 0755);
    if (mount("tmpfs", root, "tmpfs", MS_NOSUID, "mode=0755")) fatal("new root");
    const char *dirs[] = {"/system", "/vendor", "/odm", "/vendor_dlkm", "/system_dlkm",
        "/chip_prod", "/sys_prod", "/cust", "/patch_hw", "/data", "/dev", "/proc", "/sys",
        "/mnt", "/tmp", "/log", "/config", "/storage", "/sec_storage", "/module_update",
        "/preload", "/version", "/updater", "/eng_chipset", "/eng_system"};
    for (size_t i = 0; i < sizeof(dirs) / sizeof(dirs[0]); ++i) root_directory(dirs[i], 0755);
    mount_or_bind_dsu("/system", "system_gsi", true);
    root_bind("/system/bin/hmos-native-init", "/system/bin/init", true);
    mount_or_bind_dsu("/vendor", "vendor_gsi", true);
    mount_or_bind_dsu("/vendor_dlkm", "vendor_dlkm_gsi", true);
    mount_or_bind_dsu("/system_dlkm", "system_dlkm_gsi", true);
    // Reuse Android's already mounted read-only stock ODM EROFS.
    struct statvfs odm_flags;
    struct statfs odm_fs;
    struct stat odm_st, system_st;
    if (!statvfs("/odm", &odm_flags) && (odm_flags.f_flag & ST_RDONLY) &&
        !statfs("/odm", &odm_fs) && (unsigned long)odm_fs.f_type == 0xe0f5e1e2UL &&
        !stat("/odm", &odm_st) && !stat("/system", &system_st) && odm_st.st_dev != system_st.st_dev) {
        root_bind("/odm", "/odm", true);
        log_line("bound existing stock ODM read-only; no ODM image in DSU package");
    } else {
        log_line("warning: stock ODM read-only check bypassed/not strictly matching");
        root_bind("/odm", "/odm", true);
    }
    char target[512];
    mount_or_bind_dsu("/chip_prod", "chip_prod_gsi", false);
    mount_or_bind_dsu("/sys_prod", "sys_prod_gsi", false);
    mount_or_bind_dsu("/cust", "cust_gsi", false);
    mount_or_bind_dsu("/patch_hw", "patch_hw_gsi", false);
    root_link("system/bin", "/bin");
    root_link("system/etc", "/etc");
    root_link("system/lib", "/lib");
    root_link("system/lib64", "/lib64");
    root_link("system/bin/init", "/init");
    root_link("vendor", "/chipset");
    root_mount("tmpfs", "/dev", "tmpfs", MS_NOSUID, "mode=0755");
    root_directory("/dev/block", 0700);
    root_directory("/dev/unix", 0755);
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
    log_line("attempting mount of userdata_gsi onto /data...");
    if (mount(target, data_target, "ext4", MS_NOSUID | MS_NODEV, NULL) &&
        mount(pinned_data, data_target, "ext4", MS_NOSUID | MS_NODEV, NULL) &&
        mount(target, data_target, "f2fs", MS_NOSUID | MS_NODEV, NULL) &&
        mount(pinned_data, data_target, "f2fs", MS_NOSUID | MS_NODEV, NULL)) {
        log_line("unformatted DSU data partition; formatting with mke2fs ext4...");
        initialize_blank_dsu_data(target, data);
        verify_data_node(data_fd, data);
        if (mount(target, data_target, "ext4", MS_NOSUID | MS_NODEV, NULL) &&
            mount(pinned_data, data_target, "ext4", MS_NOSUID | MS_NODEV, NULL) &&
            mount(target, data_target, "ext4", 0, NULL)) {
            fatal("initialized DSU data mount");
        }
    }
    close(data_fd);
    unlink(target);
    rootpath(target, sizeof(target), "/data/hmos-port");
    directory(target, 0700);
    rootpath(target, sizeof(target), "/data/hmos-port/bootstrap.log");
    bootlog = open(target, O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0600);
    log_line("persistent data mounted exclusively from userdata_gsi");
    load_stock_modules();
    device_namespace();
    const char *brightness = "/sys/class/backlight/panel0-backlight/brightness";
    if (!access(brightness, F_OK)) {
        chown(brightness, 0, 1003);
        chmod(brightness, 0660);
        root_bind(brightness, brightness, false);
    }
    if (!access("/hmos/startup/init.cfg", F_OK)) root_bind("/hmos/startup/init.cfg", "/system/etc/init.cfg", true);
    if (!access("/hmos/startup/system-init", F_OK)) root_bind("/hmos/startup/system-init", "/system/etc/init", true);
    if (!access("/hmos/init-modules", F_OK)) root_bind("/hmos/init-modules", "/system/lib64/init", true);
    if (!access("/hmos/startup/vendor-init", F_OK)) root_bind("/hmos/startup/vendor-init", "/vendor/etc/init", true);
    mask_extra_init();
    if (!access("/hmos/startup/hdf_default.hcb", F_OK)) root_bind("/hmos/startup/hdf_default.hcb", "/chip_prod/etc/hdfconfig/hdf_default.hcb", true);
    if (!access("/hmos/policy/policy.31", F_OK)) root_bind("/hmos/policy/policy.31", "/vendor/etc/selinux/prebuild_sepolicy/policy.31", true);
    if (!access("/hmos/policy/developer_policy", F_OK)) root_bind("/hmos/policy/developer_policy", "/vendor/etc/selinux/prebuild_sepolicy/developer_policy", true);
    if (!access("/hmos/display/libdisplay_buffer_vdi_impl.z.so", F_OK)) root_bind("/hmos/display/libdisplay_buffer_vdi_impl.z.so", "/vendor/lib64/libdisplay_buffer_vdi_impl.z.so", true);
    if (!access("/hmos/display/libdisplay_composer_vdi_impl.z.so", F_OK)) root_bind("/hmos/display/libdisplay_composer_vdi_impl.z.so", "/vendor/lib64/libdisplay_composer_vdi_impl.z.so", true);
    if (!access("/hmos/ui/display_manager_config.xml", F_OK)) root_bind("/hmos/ui/display_manager_config.xml", "/sys_prod/etc/window/resources/display_manager_config.xml", true);
    if (!access("/hmos/ui/hgm_policy_config.xml", F_OK)) root_bind("/hmos/ui/hgm_policy_config.xml", "/sys_prod/etc/graphic/hgm_policy_config.xml", true);
    isolate_devices();
    prctl(PR_CAPBSET_DROP, CAP_SYS_BOOT, 0, 0, 0);
    prctl(PR_CAPBSET_DROP, CAP_SYS_RAWIO, 0, 0, 0);
    prctl(PR_CAPBSET_DROP, CAP_SYS_MODULE, 0, 0, 0);
    root_directory("/sys/fs/selinux", 0755);
    root_directory("/dev/cgroup", 0755);
    root_mount("cgroup2", "/dev/cgroup", "cgroup2", MS_NOSUID | MS_NODEV | MS_NOEXEC, NULL);
    log_line("executing full donor init --second-stage; target: SceneBoard/Ace 5");
    if (chdir(root) || chroot(".") || chdir("/")) fatal("chroot");
    syscall(__NR_close_range, 3U, ~0U, 0);
    kmsg = bootlog = -1;
    int null = open("/dev/null", O_RDWR);
    if (null >= 0) {
        for (int fd = 0; fd < 3; ++fd) dup2(null, fd);
        if (null > 2) close(null);
    }
    char *const init_args[] = {"/system/bin/init", "--second-stage", "0", NULL};
    char *const environment[] = {"PATH=/system/bin:/vendor/bin", "OHOS_MESA_SOFTWARE=true", NULL};
    execve(init_args[0], init_args, environment);
    kmsg = open("/dev/kmsg", O_WRONLY | O_CLOEXEC);
    fatal("native init exec");
}
