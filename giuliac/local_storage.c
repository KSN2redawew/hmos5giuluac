/* SPDX-License-Identifier: Apache-2.0
 * Local-only hmdfs/sharefs views for the isolated HarmonyOS storage process on OnePlus Ace 5 (giuliac).
 * Data remains in persistent /data; tmpfs holds mountpoint directories only.
 * No token, encryption, signature or integrity ioctl is intercepted.
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>

typedef int (*mount_fn)(const char *, const char *, const char *, unsigned long, const void *);
typedef int (*unmount_fn)(const char *, int);
static mount_fn native_mount;
static unmount_fn native_unmount;
static pthread_once_t once = PTHREAD_ONCE_INIT;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static char *local_mounts[128];
static const char *views[] = {"device_view/local", "merge_view", "cloud_merge_view"};

static void resolve(void)
{
    native_mount = (mount_fn)dlsym(RTLD_NEXT, "mount");
    native_unmount = (unmount_fn)dlsym(RTLD_NEXT, "umount2");
}

static bool below(const char *path, const char *parent)
{
    return !strncmp(path, parent, strlen(parent)) && path[strlen(parent)] != 0;
}

static int directory_path(const char *path, char resolved[PATH_MAX])
{
    struct stat st;
    if (!path || !realpath(path, resolved)) return -1;
    if (stat(resolved, &st) || !S_ISDIR(st.st_mode)) { errno = ENOTDIR; return -1; }
    return 0;
}

static int make_directory(const char *path)
{
    if (mkdir(path, 0711) && errno != EEXIST) return -1;
    struct stat st;
    if (lstat(path, &st) || !S_ISDIR(st.st_mode)) { errno = ENOTDIR; return -1; }
    return 0;
}

static int bind_view(const char *source, const char *target, unsigned long flags)
{
    if (native_mount(source, target, NULL, MS_BIND, NULL)) return -1;
    unsigned long attributes = flags & (MS_RDONLY | MS_NOSUID | MS_NODEV | MS_NOEXEC);
    if (attributes && native_mount(NULL, target, NULL, MS_BIND | MS_REMOUNT | attributes, NULL)) {
        int saved = errno;
        native_unmount(target, MNT_DETACH);
        errno = saved;
        return -1;
    }
    return 0;
}

int mount(const char *source, const char *target, const char *filesystemtype,
          unsigned long mountflags, const void *data)
{
    pthread_once(&once, resolve);
    if (!filesystemtype || (strcmp(filesystemtype, "hmdfs") && strcmp(filesystemtype, "sharefs")))
        return native_mount(source, target, filesystemtype, mountflags, data);
    char from[PATH_MAX], to[PATH_MAX];
    if (directory_path(source, from) || directory_path(target, to)) return -1;
    if (!below(from, "/data/service/el2/") && !below(from, "/data/service/el1/") &&
        !below(from, "/data/app/el2/") && !below(from, "/data/app/el1/")) {
        errno = EPERM;
        return -1;
    }
    pthread_mutex_lock(&lock);
    size_t slot = 0;
    while (slot < sizeof(local_mounts) / sizeof(local_mounts[0]) && local_mounts[slot]) ++slot;
    if (slot == sizeof(local_mounts) / sizeof(local_mounts[0])) {
        pthread_mutex_unlock(&lock);
        errno = ENOSPC;
        return -1;
    }
    if (!strcmp(filesystemtype, "sharefs")) {
        if (bind_view(from, to, mountflags)) {
            pthread_mutex_unlock(&lock);
            return -1;
        }
        local_mounts[slot] = strdup(to);
        pthread_mutex_unlock(&lock);
        return 0;
    }
    if (native_mount("tmpfs", to, "tmpfs", MS_NOSUID | MS_NODEV, "mode=0711")) {
        pthread_mutex_unlock(&lock);
        return -1;
    }
    size_t completed = 0;
    for (size_t i = 0; i < sizeof(views) / sizeof(views[0]); ++i) {
        char path[PATH_MAX], subdir[PATH_MAX];
        snprintf(path, sizeof(path), "%s/%s", to, views[i]);
        char *slash = strchr(views[i], '/');
        if (slash) {
            snprintf(subdir, sizeof(subdir), "%s/%.*s", to, (int)(slash - views[i]), views[i]);
            if (make_directory(subdir)) break;
        }
        if (make_directory(path) || bind_view(from, path, mountflags)) break;
        ++completed;
    }
    if (completed != sizeof(views) / sizeof(views[0])) {
        int saved = errno;
        while (completed) {
            char path[PATH_MAX];
            snprintf(path, sizeof(path), "%s/%s", to, views[--completed]);
            native_unmount(path, MNT_DETACH);
        }
        native_unmount(to, MNT_DETACH);
        pthread_mutex_unlock(&lock);
        errno = saved;
        return -1;
    }
    local_mounts[slot] = strdup(to);
    pthread_mutex_unlock(&lock);
    return 0;
}

int umount2(const char *target, int flags)
{
    pthread_once(&once, resolve);
    char path[PATH_MAX];
    if (directory_path(target, path)) return -1;
    pthread_mutex_lock(&lock);
    for (size_t i = 0; i < sizeof(local_mounts) / sizeof(local_mounts[0]); ++i) {
        if (local_mounts[i] && !strcmp(local_mounts[i], path)) {
            free(local_mounts[i]);
            local_mounts[i] = NULL;
            pthread_mutex_unlock(&lock);
            for (size_t v = 0; v < sizeof(views) / sizeof(views[0]); ++v) {
                char view_path[PATH_MAX];
                snprintf(view_path, sizeof(view_path), "%s/%s", path, views[v]);
                native_unmount(view_path, flags | MNT_DETACH);
            }
            return native_unmount(path, flags);
        }
    }
    pthread_mutex_unlock(&lock);
    return native_unmount(target, flags);
}
