// SPDX-License-Identifier: Apache-2.0
// Run ONLY in the supplied empty mount/user namespace with temporary /data.
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>

static void check_file(const char *path)
{
    int fd = open(path, O_RDONLY);
    assert(fd >= 0);
    char text[16] = {0};
    assert(read(fd, text, sizeof(text)) == 10 && !strcmp(text, "persistent"));
    close(fd);
}

int main(void)
{
    // The runner owns these empty temporary directories, never host /data.
    assert(access("/data/hmos-storage-test-namespace", F_OK) == 0);
    assert(!mkdir("/data/source", 0755));
    assert(!mkdir("/mnt/local", 0755));
    assert(!mkdir("/mnt/share", 0755));
    int file = open("/data/source/example", O_WRONLY | O_CREAT, 0600);
    assert(file >= 0 && write(file, "persistent", 10) == 10);
    close(file);
    assert(!mount("/data/source", "/mnt/local", "hmdfs", MS_NOSUID | MS_NODEV, NULL));
    check_file("/mnt/local/device_view/local/example");
    check_file("/mnt/local/merge_view/example");
    check_file("/mnt/local/cloud_merge_view/example");
    assert(mount("/data/source", "/mnt/local", "hmdfs", 0, NULL) == -1 && errno == EBUSY);
    file = open("/mnt/local/merge_view/newfile", O_CREAT | O_WRONLY, 0600);
    assert(file >= 0);
    close(file);
    assert(!access("/data/source/newfile", F_OK));
    assert(!umount("/mnt/local"));
    assert(access("/mnt/local/device_view/local/example", F_OK) == -1 && errno == ENOENT);
    check_file("/data/source/example");
    assert(!mount("/data/source", "/mnt/share", "sharefs", MS_RDONLY | MS_NODEV, NULL));
    check_file("/mnt/share/example");
    assert(open("/mnt/share/forbidden", O_WRONLY | O_CREAT, 0600) == -1 && errno == EROFS);
    assert(!umount2("/mnt/share", 0));
    assert(!mkdir("/mnt/user", 0755));
    assert(!mkdir("/mnt/user/0", 0755));
    assert(!mkdir("/mnt/user/0/nosharefs", 0755));
    assert(!mount("/data/source", "/mnt/user/0/nosharefs", NULL, MS_BIND, NULL));
    assert(!mount("/mnt/user/0/nosharefs", "/mnt/share", "sharefs", 0, NULL));
    check_file("/mnt/share/example");
    assert(!umount("/mnt/share"));
    assert(!umount("/mnt/user/0/nosharefs"));
    assert(mount("/etc", "/mnt/local", "hmdfs", 0, NULL) == -1 && errno == EPERM);
    assert(mount("/data/source/../../etc", "/mnt/local", "sharefs", 0, NULL) == -1 && errno == EPERM);
    assert(!mount("/data/source", "/mnt/local", "hmdfs", MS_RDONLY, NULL));
    check_file("/mnt/local/merge_view/example");
    assert(open("/mnt/local/merge_view/forbidden", O_WRONLY | O_CREAT, 0600) == -1 && errno == EROFS);
    assert(!umount("/mnt/local"));
    check_file("/data/source/example");
    puts("PASS local hmdfs/sharefs views, writes to backing data, teardown, read-only flags and path restrictions");
    return 0;
}
