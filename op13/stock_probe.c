/* SPDX-License-Identifier: MIT
 * Capability queries only: no mounts, formatting, token setting, Binder
 * transactions, DRM master/modesets, module loads, or device-node creation.
 */
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>
#include <unistd.h>

struct version { int32_t protocol; };
struct feature_set { uint64_t value; };
struct drm_cap { uint64_t capability, value; };
#define BINDER_VERSION_QUERY _IOWR('b', 9, struct version)
#define OHOS_FEATURE_QUERY _IOWR('b', 30, struct feature_set)
#define TOKEN_GET_QUERY _IOR('A', 1, uint64_t)
#define DRM_CAP_QUERY _IOWR('d', 0x0c, struct drm_cap)

static void exists(const char *path)
{
    errno = 0;
    int result = access(path, F_OK);
    printf("{\"kind\":\"node\",\"path\":\"%s\",\"present\":%s,\"errno\":%d}\n",
           path, result == 0 ? "true" : "false", result ? errno : 0);
}

static void binder(const char *path)
{
    int fd = open(path, O_RDWR | O_CLOEXEC);
    int error = fd < 0 ? errno : 0;
    struct version version = {0};
    struct feature_set features = {0};
    int vr = -1, fr = -1, ve = error, fe = error;
    if (fd >= 0) {
        vr = ioctl(fd, BINDER_VERSION_QUERY, &version);
        ve = vr ? errno : 0;
        fr = ioctl(fd, OHOS_FEATURE_QUERY, &features);
        fe = fr ? errno : 0;
        close(fd);
    }
    printf("{\"kind\":\"binder\",\"path\":\"%s\",\"open_errno\":%d,"
           "\"version_errno\":%d,\"protocol\":%d,\"ohos_features_errno\":%d,"
           "\"ohos_features\":%" PRIu64 "}\n",
           path, error, ve, version.protocol, fe, features.value);
}

static void drm(const char *path)
{
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    int error = fd < 0 ? errno : 0;
    /* GET_CAP(DUMB_BUFFER) does not allocate a buffer or change display state. */
    struct drm_cap cap = {1, 0};
    int result = fd < 0 ? -1 : ioctl(fd, DRM_CAP_QUERY, &cap);
    int query_error = result ? (fd < 0 ? error : errno) : 0;
    if (fd >= 0) close(fd);
    printf("{\"kind\":\"drm\",\"path\":\"%s\",\"open_errno\":%d,"
           "\"dumb_cap_errno\":%d,\"dumb_buffers\":%" PRIu64 "}\n",
           path, error, query_error, cap.value);
}

int main(void)
{
    /* Fixed strings and numeric values keep the output valid JSONL even when
     * the device has an arbitrary kernel release or command line. */
    puts("{\"kind\":\"scope\",\"schema\":1,\"read_only_queries\":true,"
         "\"proves_hmos_boot\":false}");
    printf("{\"kind\":\"process\",\"uid\":%u,\"page_size\":%ld}\n",
           (unsigned)getuid(), sysconf(_SC_PAGESIZE));
    const char *paths[] = {
        "/dev/binder", "/dev/hwbinder", "/dev/vndbinder",
        "/dev/binderfs/binder", "/dev/access_token_id", "/dev/kgsl-3d0",
        "/dev/dma_heap/system", "/dev/dma_heap/qcom,system",
        "/dev/dma_heap/qcom,display", "/sys/class/drm/card0-DSI-1",
        "/sys/class/misc/access_token_id", "/sys/fs/selinux/enforce"
    };
    for (unsigned i = 0; i < sizeof(paths) / sizeof(paths[0]); i++) exists(paths[i]);
    binder("/dev/binder");
    binder("/dev/binderfs/binder");
    int fd = open("/dev/access_token_id", O_RDONLY | O_CLOEXEC);
    int open_error = fd < 0 ? errno : 0;
    uint64_t token = 0;
    int result = fd < 0 ? -1 : ioctl(fd, TOKEN_GET_QUERY, &token);
    int query_error = result ? (fd < 0 ? open_error : errno) : 0;
    if (fd >= 0) close(fd);
    /* Record support, not the caller's token value. */
    printf("{\"kind\":\"token\",\"open_errno\":%d,\"get_errno\":%d}\n",
           open_error, query_error);
    drm("/dev/dri/card0");
    drm("/dev/dri/card1");
    return ferror(stdout) ? 1 : 0;
}
