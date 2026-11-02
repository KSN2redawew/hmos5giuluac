/* SPDX-License-Identifier: Apache-2.0
 * Local-only hmdfs/sharefs views for the isolated HarmonyOS storage process.
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

int mount(const char *source, const char *target, const char *type, unsigned long flags, const void *options)
{
    pthread_once(&once, resolve);
    if (!native_mount || !native_unmount) { errno = ENOSYS; return -1; }
    if (!type || (strcmp(type, "hmdfs") && strcmp(type, "sharefs")) ||
        (flags & (MS_BIND | MS_REMOUNT | MS_MOVE | MS_SHARED | MS_PRIVATE | MS_SLAVE | MS_UNBINDABLE)))
        return native_mount(source, target, type, flags, options);
    char from[PATH_MAX], to[PATH_MAX];
    if (directory_path(source, from) || directory_path(target, to)) return -1;
    bool source_allowed = below(from, "/data/") ||
        (!strcmp(type, "sharefs") && below(from, "/mnt/user/"));
    if (!source_allowed || !(below(to, "/data/") || below(to, "/mnt/") || below(to, "/storage/"))) {
        errno = EPERM; return -1;
    }
    if (!strcmp(type, "sharefs")) return bind_view(from, to, flags);
    pthread_mutex_lock(&lock);
    size_t slot = 128;
    for (size_t i = 0; i < 128; ++i) {
        if (local_mounts[i] && !strcmp(local_mounts[i], to)) {
            pthread_mutex_unlock(&lock); errno = EBUSY; return -1;
        }
        if (!local_mounts[i] && slot == 128) slot = i;
    }
    if (slot == 128) { pthread_mutex_unlock(&lock); errno = ENOSPC; return -1; }
    char *record = strdup(to);
    if (!record) { pthread_mutex_unlock(&lock); return -1; }
    int result = native_mount("op13-local-hmdfs", to, "tmpfs", MS_NOSUID | MS_NODEV | MS_NOEXEC, "mode=0711");
    unsigned completed = 0;
    if (!result) {
        char path[PATH_MAX];
        if (snprintf(path, sizeof(path), "%s/device_view", to) >= (int)sizeof(path) || make_directory(path)) {
            result = -1;
        }
        for (unsigned i = 0; !result && i < 3; ++i) {
            if (snprintf(path, sizeof(path), "%s/%s", to, views[i]) >= (int)sizeof(path)) {
                errno = ENAMETOOLONG; result = -1; break;
            }
            if (make_directory(path) || bind_view(from, path, flags)) {
                result = -1; break;
            }
            ++completed;
        }
        if (!result && (flags & MS_RDONLY))
            result = native_mount(NULL, to, "tmpfs", MS_REMOUNT | MS_RDONLY | MS_NOSUID | MS_NODEV | MS_NOEXEC, NULL);
        if (result) {
            int saved = errno;
            while (completed) {
                snprintf(path, sizeof(path), "%s/%s", to, views[--completed]);
                native_unmount(path, MNT_DETACH);
            }
            native_unmount(to, MNT_DETACH);
            errno = saved;
        }
    }
    int saved = errno;
    if (!result) local_mounts[slot] = record;
    else free(record);
    pthread_mutex_unlock(&lock);
    errno = saved;
    return result;
}

int umount2(const char *target, int flags)
{
    pthread_once(&once, resolve);
    if (!native_unmount) { errno = ENOSYS; return -1; }
    char path[PATH_MAX];
    if (!realpath(target, path)) return -1;
    pthread_mutex_lock(&lock);
    for (size_t slot = 0; slot < 128; ++slot) {
        if (!local_mounts[slot] || strcmp(local_mounts[slot], path)) continue;
        for (unsigned i = 3; i > 0; --i) {
            char child[PATH_MAX];
            if (snprintf(child, sizeof(child), "%s/%s", path, views[i - 1]) >= (int)sizeof(child)) {
                pthread_mutex_unlock(&lock); errno = ENAMETOOLONG; return -1;
            }
            if (native_unmount(child, flags) && errno != EINVAL && errno != ENOENT) {
                int saved = errno; pthread_mutex_unlock(&lock); errno = saved; return -1;
            }
        }
        int result = native_unmount(path, flags);
        int saved = errno;
        if (!result) { free(local_mounts[slot]); local_mounts[slot] = NULL; }
        pthread_mutex_unlock(&lock);
        errno = saved;
        return result;
    }
    pthread_mutex_unlock(&lock);
    return native_unmount(target, flags);
}

int umount(const char *target)
{
    return umount2(target, 0);
}
