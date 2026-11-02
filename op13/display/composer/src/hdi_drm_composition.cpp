/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "hdi_drm_composition.h"
#include <cstdio>
#include <cstring>
#include <map>
#include <thread>
#include <fcntl.h>
#include <sys/mman.h>
#include <poll.h>
#include <xf86drmMode.h>
#include <drm_fourcc.h>
#include <string>
#include <functional>
#include <drm.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/dma-buf.h>
#include <time.h>
#include <unistd.h>
#include <dlfcn.h>
#include <atomic>
#include "hmos_text.h"
#include <cerrno>
#include "hdi_drm_layer.h"

namespace OHOS {
namespace HDI {
namespace DISPLAY {
HdiDrmComposition::HdiDrmComposition(const std::shared_ptr<DrmConnector> &connector,
                                     const std::shared_ptr<DrmCrtc> &crtc,
                                     const std::shared_ptr<DrmDevice> &drmDevice)
    : mDrmDevice(drmDevice), mConnector(connector), mCrtc(crtc)
{
    DISPLAY_LOGD();
}


std::atomic<bool> g_hmosDisplayOff{false};
std::atomic<uint64_t> g_op13VsyncPeriodNs{16666667};

/* bootevent.boot.completed is set by init internally (no kmsg line), so ask
 * the parameter service directly; checked at most once per second */
static bool HmosBootCompleted()
{
    using GetParamFn = int (*)(const char *, const char *, char *, uint32_t);
    static GetParamFn fn = nullptr;
    static bool tried = false;
    static int64_t last = 0;
    if (!tried) {
        tried = true;
        void *h = dlopen("libbegetutil.z.so", RTLD_NOW);
        if (h != nullptr) {
            fn = reinterpret_cast<GetParamFn>(dlsym(h, "GetParameter"));
        }
    }
    if (fn == nullptr) {
        return false;
    }
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    int64_t now = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    if (now - last < 1000) {
        return false;
    }
    last = now;
    char v[16] = {0};
    return fn("bootevent.boot.completed", "false", v, sizeof(v)) > 0 && strcmp(v, "true") == 0;
}

/* ---------------- sweet boot verbose overlay (hmos.verbose=1) ---------------- */
static bool HmosVerboseEnabled()
{
    static int enabled = -1;
    if (enabled < 0) {
        enabled = 0;
        char buf[2048] = {0};
        int fd = open("/proc/cmdline", O_RDONLY | O_CLOEXEC);
        if (fd >= 0) {
            ssize_t n = read(fd, buf, sizeof(buf) - 1);
            close(fd);
            if (n > 0 && strstr(buf, "hmos.verbose=1") != nullptr) {
                enabled = 1;
            }
        }
    }
    return enabled == 1;
}

static int64_t HmosNowMs()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<int64_t>(ts.tv_sec) * 1000 + ts.tv_nsec / 1000000;
}

/*
 * The overlay is never drawn into render_service's buffer (that would pile up
 * text on frames that get re-shown, and darken them over and over).  Instead
 * every frame is copied into one of three composer-owned scanout buffers, the
 * log is blended on top of the copy, and that copy is what gets scanned out.
 */
struct HmosShadow {
    uint32_t handle = 0, fb = 0, pitch = 0;
    uint8_t *map = nullptr;
    size_t size = 0;
};
static HmosShadow g_shadow[3];
static int g_shadowIdx = 0, g_shadowW = 0, g_shadowH = 0;
static uint32_t g_shadowFmt = 0, g_overrideFb = 0;

static void HmosShadowFree(int drmFd)
{
    for (auto &b : g_shadow) {
        if (b.map) munmap(b.map, b.size);
        if (b.fb) drmModeRmFB(drmFd, b.fb);
        if (b.handle) {
            struct drm_mode_destroy_dumb dd = {b.handle};
            ioctl(drmFd, DRM_IOCTL_MODE_DESTROY_DUMB, &dd);
        }
        b = HmosShadow();
    }
    g_shadowW = g_shadowH = 0;
}

static bool HmosShadowAlloc(int drmFd, int w, int h, uint32_t fmt)
{
    if (g_shadowW == w && g_shadowH == h && g_shadowFmt == fmt) {
        return true;
    }
    HmosShadowFree(drmFd);
    for (auto &b : g_shadow) {
        struct drm_mode_create_dumb cd = {};
        cd.width = w;
        cd.height = h;
        cd.bpp = 32;
        if (ioctl(drmFd, DRM_IOCTL_MODE_CREATE_DUMB, &cd)) goto fail;
        b.handle = cd.handle;
        b.pitch = cd.pitch;
        b.size = cd.size;
        uint32_t handles[4] = {cd.handle}, pitches[4] = {cd.pitch}, offsets[4] = {0};
        if (drmModeAddFB2(drmFd, w, h, fmt, handles, pitches, offsets, &b.fb, 0)) goto fail;
        struct drm_mode_map_dumb md = {};
        md.handle = cd.handle;
        if (ioctl(drmFd, DRM_IOCTL_MODE_MAP_DUMB, &md)) goto fail;
        void *m = mmap(nullptr, b.size, PROT_READ | PROT_WRITE, MAP_SHARED, drmFd, md.offset);
        if (m == MAP_FAILED) goto fail;
        b.map = static_cast<uint8_t *>(m);
    }
    g_shadowW = w;
    g_shadowH = h;
    g_shadowFmt = fmt;
    DISPLAY_LOGI("hmos verbose: 3 overlay scanout buffers %{public}dx%{public}d", w, h);
    return true;
fail:
    DISPLAY_LOGE("hmos verbose: overlay buffer alloc failed errno %{public}d", errno);
    HmosShadowFree(drmFd);
    return false;
}


/*
 * Verbose on a separate hardware plane: the log is drawn into ARGB buffers
 * that sit above the client layer (zpos 1, premultiplied alpha), so the boot
 * animation frames are never copied (the old copy of each 10 MB frame out of
 * uncached memory capped the animation at ~14 fps).  The text buffer is
 * redrawn only when the log changes, at most 4 times a second.
 */
static bool g_ovlBroken = false;
static int g_ovlIdx = 0;
static uint32_t g_ovlFb = 0;
static int64_t g_ovlLastDraw = 0;
static size_t g_ovlLastHash = 0;

static uint32_t HmosOverlayFb(int drmFd, int w, int h)
{
    if (g_ovlBroken || !HmosShadowAlloc(drmFd, w, h, DRM_FORMAT_ARGB8888)) {
        return 0;
    }
    int64_t now = HmosNowMs();
    if (g_ovlFb != 0 && now - g_ovlLastDraw < 250) {
        return g_ovlFb;
    }
    static char text[48 * 1024];
    int tfd = open("/dev/hmos_verbose", O_RDONLY | O_CLOEXEC);
    ssize_t n = 0;
    if (tfd >= 0) {
        n = read(tfd, text, sizeof(text) - 1);
        close(tfd);
    }
    if (n <= 0) {
        n = snprintf(text, sizeof(text), "hmos verbose: waiting for /dev/hmos_verbose");
    }
    text[n] = 0;
    g_ovlLastDraw = now;
    size_t hash = std::hash<std::string>()(std::string(text, n));
    if (g_ovlFb != 0 && hash == g_ovlLastHash) {
        return g_ovlFb;
    }
    g_ovlLastHash = hash;
    HmosShadow &dst = g_shadow[g_ovlIdx];
    g_ovlIdx = (g_ovlIdx + 1) % 3;
    /* premultiplied black at ~40%: the animation stays visible behind the text */
    for (int y = 0; y < h; y++) {
        uint32_t *line = reinterpret_cast<uint32_t *>(dst.map + static_cast<size_t>(y) * dst.pitch);
        for (int x = 0; x < w; x++) {
            line[x] = 0x66000000;
        }
    }
    hmos_draw_screen(dst.map, dst.pitch, w, h, text, 0);
    g_ovlFb = dst.fb;
    return g_ovlFb;
}

static uint64_t HmosPlaneEnum(int fd, uint32_t planeId, const char *prop, const char *value, uint32_t *propId)
{
    *propId = 0;
    drmModeObjectPropertiesPtr props = drmModeObjectGetProperties(fd, planeId, DRM_MODE_OBJECT_PLANE);
    if (props == nullptr) {
        return 0;
    }
    uint64_t v = 0;
    for (uint32_t i = 0; i < props->count_props && *propId == 0; i++) {
        drmModePropertyPtr pp = drmModeGetProperty(fd, props->props[i]);
        if (pp == nullptr) {
            continue;
        }
        if (strcmp(pp->name, prop) == 0) {
            for (int e = 0; e < pp->count_enums; e++) {
                if (strcmp(pp->enums[e].name, value) == 0) {
                    *propId = pp->prop_id;
                    v = pp->enums[e].value;
                }
            }
        }
        drmModeFreeProperty(pp);
    }
    drmModeFreeObjectProperties(props);
    return v;
}

/* returns the fb id to scan out instead of the client buffer, or 0 */
static uint32_t HmosComposeOverlay(int drmFd, HdiLayerBuffer *buf, int acquireFence)
{
    if (buf == nullptr) {
        return 0;
    }
    int32_t fmt = buf->GetFormat();
    if (fmt != PIXEL_FMT_RGBA_8888 && fmt != PIXEL_FMT_RGBX_8888 && fmt != PIXEL_FMT_BGRA_8888 &&
        fmt != PIXEL_FMT_BGRX_8888) {
        return 0;
    }
    int w = buf->GetWight();
    int h = buf->GetHeight();
    if (!HmosShadowAlloc(drmFd, w, h, DrmDevice::ConvertToDrmFormat(static_cast<PixelFormat>(fmt)))) {
        return 0;
    }
    int fd = buf->GetFb();
    struct stat st;
    if (fd < 0 || fstat(fd, &st) != 0) {
        return 0;
    }
    static std::map<ino_t, std::pair<void *, size_t>> maps;
    size_t size = static_cast<size_t>(buf->mHandle.size);
    auto it = maps.find(st.st_ino);
    void *addr = nullptr;
    if (it != maps.end() && it->second.second == size) {
        addr = it->second.first;
    } else {
        addr = mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0);
        if (addr == MAP_FAILED) {
            return 0;
        }
        if (maps.size() > 16) {
            for (auto &m : maps) {
                munmap(m.second.first, m.second.second);
            }
            maps.clear();
        }
        maps[st.st_ino] = {addr, size};
    }
    if (acquireFence >= 0) { /* wait until render_service finished the frame */
        struct pollfd pfd = {acquireFence, POLLIN, 0};
        poll(&pfd, 1, 200);
    }
    static char text[48 * 1024];
    int tfd = open("/dev/hmos_verbose", O_RDONLY | O_CLOEXEC);
    ssize_t n = 0;
    if (tfd >= 0) {
        n = read(tfd, text, sizeof(text) - 1);
        close(tfd);
    }
    if (n <= 0) {
        n = snprintf(text, sizeof(text), "hmos verbose: waiting for /dev/hmos_verbose");
    }
    text[n] = 0;
    HmosShadow &dst = g_shadow[g_shadowIdx];
    g_shadowIdx = (g_shadowIdx + 1) % 3;
    struct dma_buf_sync s1 = {DMA_BUF_SYNC_START | DMA_BUF_SYNC_READ};
    ioctl(fd, DMA_BUF_IOCTL_SYNC, &s1);
    int srcStride = buf->GetStride();
    size_t row = static_cast<size_t>(w) * 4;
    for (int y = 0; y < h; y++) {
        memcpy(dst.map + static_cast<size_t>(y) * dst.pitch,
               static_cast<uint8_t *>(addr) + static_cast<size_t>(y) * srcStride, row);
    }
    struct dma_buf_sync s2 = {DMA_BUF_SYNC_END | DMA_BUF_SYNC_READ};
    ioctl(fd, DMA_BUF_IOCTL_SYNC, &s2);
    hmos_draw_screen(dst.map, dst.pitch, w, h, text, 1);
    return dst.fb;
}

void HdiDrmComposition::VerboseKeepAlive()
{
    /* when render_service stops sending frames, keep refreshing the log */
    for (;;) {
        usleep(1000000);
        std::lock_guard<std::mutex> lock(mApplyMutex);
        if (mCompLayers.size() == 1 && HmosNowMs() - mLastApplyMs > 1000) {
            ApplyLocked(false);
        }
    }
}

int32_t HdiDrmComposition::Init()
{
    DISPLAY_LOGD();
    mPrimPlanes.clear();
    mOverlayPlanes.clear();
    mPlanes.clear();
    DISPLAY_CHK_RETURN((mCrtc == nullptr), DISPLAY_FAILURE, DISPLAY_LOGE("crtc is null"));
    DISPLAY_CHK_RETURN((mConnector == nullptr), DISPLAY_FAILURE, DISPLAY_LOGE("connector is null"));
    DISPLAY_CHK_RETURN((mDrmDevice == nullptr), DISPLAY_FAILURE, DISPLAY_LOGE("drmDevice is null"));
    mPrimPlanes = mDrmDevice->GetDrmPlane(mCrtc->GetPipe(), DRM_PLANE_TYPE_PRIMARY);
    mOverlayPlanes = mDrmDevice->GetDrmPlane(mCrtc->GetPipe(), DRM_PLANE_TYPE_OVERLAY);
    mPlanes.insert(mPlanes.end(), mPrimPlanes.begin(), mPrimPlanes.end());
    mPlanes.insert(mPlanes.end(), mOverlayPlanes.begin(), mOverlayPlanes.end());
    return DISPLAY_SUCCESS;
}

int32_t HdiDrmComposition::SetLayers(std::vector<HdiLayer *> &layers, HdiLayer &clientLayer)
{
    // now we do not surpport present direct
    DISPLAY_LOGD();
    std::lock_guard<std::mutex> lock(mApplyMutex);
    mCompLayers.clear();
    mCompLayers.push_back(&clientLayer);
    return DISPLAY_SUCCESS;
}

int32_t HdiDrmComposition::SetCrtcProperty(DrmPlane &drmPlane,
                                           drmModeAtomicReqPtr pset,
                                           int32_t bufferW,
                                           int32_t bufferH)
{
    int ret;

    ret = drmModeAtomicAddProperty(pset, drmPlane.GetId(), drmPlane.GetPropCrtc_xId(), 0);
    DISPLAY_LOGD("set the fb planeid %{public}d, GetPropCrtc_xId %{public}d, crop.x %{public}d", drmPlane.GetId(),
        drmPlane.GetPropCrtc_xId(), 0);
    DISPLAY_CHK_RETURN((ret < 0), DISPLAY_FAILURE, DISPLAY_LOGE("set the fb planeid fialed errno : %{public}d", errno));

    ret = drmModeAtomicAddProperty(pset, drmPlane.GetId(), drmPlane.GetPropCrtc_yId(), 0);
    DISPLAY_LOGD("set the fb planeid %{public}d, GetPropCrtc_yId %{public}d, crop.y %{public}d", drmPlane.GetId(),
        drmPlane.GetPropCrtc_yId(), 0);
    DISPLAY_CHK_RETURN((ret < 0), DISPLAY_FAILURE, DISPLAY_LOGE("set the fb planeid fialed errno : %{public}d", errno));

    ret = drmModeAtomicAddProperty(pset, drmPlane.GetId(), drmPlane.GetPropCrtc_wId(), bufferW);
    DISPLAY_LOGD("set the fb planeid %{public}d, GetPropCrtc_wId %{public}d, crop.w %{public}d", drmPlane.GetId(),
        drmPlane.GetPropCrtc_wId(), bufferW);
    DISPLAY_CHK_RETURN((ret < 0), DISPLAY_FAILURE, DISPLAY_LOGE("set the fb planeid fialed errno : %{public}d", errno));

    ret = drmModeAtomicAddProperty(pset, drmPlane.GetId(), drmPlane.GetPropCrtc_hId(), bufferH);
    DISPLAY_LOGD("set the fb planeid %{public}d, GetPropCrtc_hId %{public}d, crop.h %{public}d", drmPlane.GetId(),
        drmPlane.GetPropCrtc_xId(), bufferH);
    DISPLAY_CHK_RETURN((ret < 0), DISPLAY_FAILURE, DISPLAY_LOGE("set the fb planeid fialed errno : %{public}d", errno));

    return DISPLAY_SUCCESS;
}

int32_t HdiDrmComposition::SetSrcProperty(DrmPlane &drmPlane,
                                          drmModeAtomicReqPtr pset,
                                          int32_t bufferW,
                                          int32_t bufferH)
{
    int ret;

    ret = drmModeAtomicAddProperty(pset, drmPlane.GetId(), drmPlane.GetPropSrc_xId(), 0 << 16); // 16:shift left 16 bits
    DISPLAY_LOGD("set the fb planeid %{public}d, GetPropSrc_xId %{public}d, displayRect.x %{public}d",
        drmPlane.GetId(), drmPlane.GetPropSrc_xId(), 0);
    DISPLAY_CHK_RETURN((ret < 0), DISPLAY_FAILURE, DISPLAY_LOGE("set the fb planeid fialed errno : %{public}d", errno));

    ret = drmModeAtomicAddProperty(pset, drmPlane.GetId(), drmPlane.GetPropSrc_yId(), 0 << 16); // 16:shift left 16 bits
    DISPLAY_LOGD("set the fb planeid %{public}d, GetPropSrc_yId %{public}d, displayRect.y %{public}d",
        drmPlane.GetId(), drmPlane.GetPropSrc_yId(), 0);
    DISPLAY_CHK_RETURN((ret < 0), DISPLAY_FAILURE, DISPLAY_LOGE("set the fb planeid fialed errno : %{public}d", errno));

    ret = drmModeAtomicAddProperty(pset, drmPlane.GetId(), drmPlane.GetPropSrc_wId(),
                                   bufferW << 16); // 16:shift left 16 bits
    DISPLAY_LOGD("set the fb planeid %{public}d, GetPropCrtc_wId %{public}d, displayRect.w %{public}d",
        drmPlane.GetId(), drmPlane.GetPropSrc_wId(), bufferW);
    DISPLAY_CHK_RETURN((ret < 0), DISPLAY_FAILURE, DISPLAY_LOGE("set the fb planeid fialed errno : %{public}d", errno));

    ret = drmModeAtomicAddProperty(pset, drmPlane.GetId(), drmPlane.GetPropSrc_hId(),
        bufferH << 16); // 16:shift left 16 bits
    DISPLAY_LOGD("set the fb planeid %{public}d, GetPropSrc_hId %{public}d, displayRect.h %{public}d",
        drmPlane.GetId(), drmPlane.GetPropSrc_hId(), bufferH);
    DISPLAY_CHK_RETURN((ret < 0), DISPLAY_FAILURE, DISPLAY_LOGE("set the fb planeid fialed errno : %{public}d", errno));

    return DISPLAY_SUCCESS;
}

int32_t HdiDrmComposition::ApplyPlane(HdiDrmLayer &layer,
                                      HdiLayer &hlayer,
                                      DrmPlane &drmPlane,
                                      drmModeAtomicReqPtr pset)
{
    int ret;
    int fenceFd = layer.GetAcquireFenceFd();
    int propId = drmPlane.GetPropFenceInId();
    HdiLayerBuffer *layerBuffer = hlayer.GetCurrentBuffer();
    int32_t bufferW = layerBuffer->GetWight();
    int32_t bufferH = layerBuffer->GetHeight();

    DISPLAY_LOGD();
    if (propId != 0 && g_overrideFb == 0) { /* overlay path already waited for the fence */
        DISPLAY_LOGD("set the fence in prop");
        if (fenceFd >= 0) {
            ret = drmModeAtomicAddProperty(pset, drmPlane.GetId(), propId, fenceFd);
            DISPLAY_LOGD("set the IfenceProp plane id %{public}d, propId %{public}d, fenceFd %{public}d",
                drmPlane.GetId(), propId, fenceFd);
            DISPLAY_CHK_RETURN((ret < 0), DISPLAY_FAILURE, DISPLAY_LOGE("set IN_FENCE_FD failed"));
        }
    }

    ret = SetCrtcProperty(drmPlane, pset, bufferW, bufferH);
    DISPLAY_CHK_RETURN((ret < 0), DISPLAY_FAILURE, DISPLAY_LOGE("set Crtc fialed errno : %{public}d", errno));

    ret = SetSrcProperty(drmPlane, pset, bufferW, bufferH);
    DISPLAY_CHK_RETURN((ret < 0), DISPLAY_FAILURE, DISPLAY_LOGE("set Src fialed errno : %{public}d", errno));

    /* only the client layer is composed: it is always the bottom stage. The
     * layer z-order from render_service is outside the SDE zpos range and
     * makes the whole commit fail with EINVAL. */
    uint64_t zpos = mCompLayers.size() == 1 ? 0 : static_cast<uint64_t>(layer.GetZorder()) & 0xf;
    ret = drmModeAtomicAddProperty(pset, drmPlane.GetId(), drmPlane.GetPropZposId(), zpos);
    DISPLAY_LOGD("set the fb planeid %{public}d, GetPropZposId %{public}d, zpos %{public}d",
        drmPlane.GetId(), drmPlane.GetPropZposId(), layer.GetZorder());
    DISPLAY_CHK_RETURN((ret < 0), DISPLAY_FAILURE, DISPLAY_LOGE("set the zpos fialed errno : %{public}d", errno));

    // set fb id
    DrmGemBuffer *gemBuffer = layer.GetGemBuffer();
    DISPLAY_CHK_RETURN((gemBuffer == nullptr), DISPLAY_FAILURE, DISPLAY_LOGE("current gemBuffer is nullptr"));
    DISPLAY_CHK_RETURN((!gemBuffer->IsValid()), DISPLAY_FAILURE, DISPLAY_LOGE("the DrmGemBuffer is invalid"));
    uint32_t fbId = g_overrideFb ? g_overrideFb : gemBuffer->GetFbId();
    mLastPlaneFb = drmPlane.GetId();
    ret = drmModeAtomicAddProperty(pset, drmPlane.GetId(), drmPlane.GetPropFbId(), fbId);
    DISPLAY_LOGD("set the fb planeid %{public}d, propId %{public}d, fbId %{public}d",
        drmPlane.GetId(), drmPlane.GetPropFbId(), gemBuffer->GetFbId());
    DISPLAY_CHK_RETURN((ret < 0), DISPLAY_FAILURE, DISPLAY_LOGE("set fb id fialed errno : %{public}d", errno));

    // set crtc id
    ret = drmModeAtomicAddProperty(pset, drmPlane.GetId(), drmPlane.GetPropCrtcId(), mCrtc->GetId());
    DISPLAY_LOGD("set the crtc planeId %{public}d, propId %{public}d, crtcId %{public}d",
        drmPlane.GetId(), drmPlane.GetPropCrtcId(), mCrtc->GetId());
    DISPLAY_CHK_RETURN((ret < 0), DISPLAY_FAILURE, DISPLAY_LOGE("set crtc id fialed errno : %{public}d", errno));
    return DISPLAY_SUCCESS;
}

int32_t HdiDrmComposition::UpdateMode(std::unique_ptr<DrmModeBlock> &modeBlock)
{
    // set the mode
    DISPLAY_LOGD();
    if (mCrtc->NeedModeSet()) {
        int drmFd = mDrmDevice->GetDrmFd();
        drmModeAtomicReqPtr pset = drmModeAtomicAlloc();
        modeBlock = mConnector->GetModeBlockFromId(mCrtc->GetActiveModeId());
        if ((modeBlock != nullptr) && (modeBlock->GetBlockId() != DRM_INVALID_ID)) {
            // set to active
            DISPLAY_LOGD("set crtc to active");
            int ret = drmModeAtomicAddProperty(pset, mCrtc->GetId(), mCrtc->GetActivePropId(), 1);
            DISPLAY_CHK_RETURN((ret < 0), DISPLAY_FAILURE,
                DISPLAY_LOGE("can not add the active prop errno %{public}d", errno));

            // set the mode id
            DISPLAY_LOGD("set the mode");
            ret = drmModeAtomicAddProperty(pset, mCrtc->GetId(), mCrtc->GetModePropId(), modeBlock->GetBlockId());
            DISPLAY_LOGD("set the mode planeId %{public}d, propId %{public}d, GetBlockId: %{public}d",
                mCrtc->GetId(), mCrtc->GetModePropId(), modeBlock->GetBlockId());
            DISPLAY_CHK_RETURN((ret < 0), DISPLAY_FAILURE,
                DISPLAY_LOGE("can not add the mode prop errno %{public}d", errno));
            ret = drmModeAtomicAddProperty(pset, mConnector->GetId(), mConnector->GetPropCrtcId(), mCrtc->GetId());
            DISPLAY_LOGD("set the connector id: %{public}d, propId %{public}d, crtcId %{public}d",
                mConnector->GetId(), mConnector->GetPropCrtcId(), mCrtc->GetId());
            DISPLAY_CHK_RETURN((ret < 0), DISPLAY_FAILURE,
                DISPLAY_LOGE("can not add the crtc id prop %{public}d", errno));

            uint32_t flags = DRM_MODE_ATOMIC_ALLOW_MODESET;
            ret = drmModeAtomicCommit(drmFd, pset, flags, nullptr);
            DISPLAY_CHK_RETURN((ret != 0), DISPLAY_FAILURE,
                DISPLAY_LOGE("drmModeAtomicCommit failed %{public}d errno %{public}d", ret, errno));
            mCrtc->ClearModeSet();
        }
    }
    return DISPLAY_SUCCESS;
}

int32_t HdiDrmComposition::RemoveUnusePlane(drmModeAtomicReqPtr pset)
{
    int ret = 0;
    /* Remove useless planes from the drm */
    for (uint32_t j = 0; j < mPlanes.size(); j++) {
        auto &drmPlane = mPlanes[j];
        if ((static_cast<int>(drmPlane->GetWinType()) & mCrtc->GetPlaneMask()) &&  drmPlane->GetPipe() == 0) {
            DISPLAY_LOGD("no used plane %{public}s id %{public}d", drmPlane->GetName().c_str(), drmPlane->GetId());
            ret = drmModeAtomicAddProperty(pset, drmPlane->GetId(), drmPlane->GetPropFbId(), 0);
            /* FB and CRTC must be cleared together or the atomic check fails */
            ret = drmModeAtomicAddProperty(pset, drmPlane->GetId(), drmPlane->GetPropCrtcId(), 0);
            drmPlane->UnBindPipe();
        }
    }
    return DISPLAY_SUCCESS;
}

int32_t HdiDrmComposition::FindPlaneAndApply(drmModeAtomicReqPtr pset)
{
    int32_t ret = 0;
    for (uint32_t i = 0; i < mCompLayers.size(); i++) {
        HdiDrmLayer *layer = static_cast<HdiDrmLayer *>(mCompLayers[i]);
        HdiLayer *hlayer = mCompLayers[i];
        for (uint32_t j = 0; j < mPlanes.size(); j++) {
            auto &drmPlane = mPlanes[j];
            if (drmPlane->GetPipe() != 0 && drmPlane->GetPipe() != (1 << mCrtc->GetPipe())) {
                DISPLAY_LOGD("plane %{public}d used pipe %{public}d crtc pipe %{public}d", drmPlane->GetId(),
                    drmPlane->GetPipe(), mCrtc->GetPipe());
                continue;
            }
            /* Check whether the plane belond to the crtc */
            if (!(static_cast<int>(drmPlane->GetWinType()) & mCrtc->GetPlaneMask())) {
                continue;
            }
            DISPLAY_LOGD("use plane %{public}d WinType %{public}x crtc %{public}d PlaneMask %{public}x",
                drmPlane->GetId(), drmPlane->GetWinType(), mCrtc->GetId(), mCrtc->GetPlaneMask());

            if (drmPlane->GetCrtcId() == mCrtc->GetId() || drmPlane->GetCrtcId() == 0) {
                ret = ApplyPlane(*layer, *hlayer, *drmPlane, pset);
                DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), ret, DISPLAY_LOGE("apply plane failed"));
                /* mark the plane is used by crtc */
                drmPlane->BindToPipe(1 << mCrtc->GetPipe());
                break;
            }
        }
    }
    return DISPLAY_SUCCESS;
}

/* one-shot: find out which part of the atomic request the kernel rejects */
void HdiDrmComposition::DiagnoseCommit(uint32_t fbId, int w, int h)
{
    int fd = mDrmDevice->GetDrmFd();
    std::shared_ptr<DrmPlane> plane;
    for (auto &p : mPlanes) {
        if (p->GetId() == mLastPlaneFb) plane = p;
    }
    if (!plane) {
        DISPLAY_LOGE("diag: no plane");
        return;
    }
    auto test = [&](int step, const char *what) {
        drmModeAtomicReqPtr r = drmModeAtomicAlloc();
        uint32_t pid = plane->GetId();
        drmModeAtomicAddProperty(r, pid, plane->GetPropFbId(), fbId);
        drmModeAtomicAddProperty(r, pid, plane->GetPropCrtcId(), mCrtc->GetId());
        drmModeAtomicAddProperty(r, pid, plane->GetPropSrc_xId(), 0);
        drmModeAtomicAddProperty(r, pid, plane->GetPropSrc_yId(), 0);
        drmModeAtomicAddProperty(r, pid, plane->GetPropSrc_wId(), (uint64_t)w << 16);
        drmModeAtomicAddProperty(r, pid, plane->GetPropSrc_hId(), (uint64_t)h << 16);
        drmModeAtomicAddProperty(r, pid, plane->GetPropCrtc_xId(), 0);
        drmModeAtomicAddProperty(r, pid, plane->GetPropCrtc_yId(), 0);
        drmModeAtomicAddProperty(r, pid, plane->GetPropCrtc_wId(), w);
        drmModeAtomicAddProperty(r, pid, plane->GetPropCrtc_hId(), h);
        if (step >= 1) drmModeAtomicAddProperty(r, pid, plane->GetPropZposId(), 0);
        uint64_t outFence = -1;
        if (step >= 2) drmModeAtomicAddProperty(r, mCrtc->GetId(), mCrtc->GetOutFencePropId(), (uint64_t)&outFence);
        if (step >= 3) drmModeAtomicAddProperty(r, mConnector->GetId(), mConnector->GetPropCrtcId(), mCrtc->GetId());
        if (step >= 4) drmModeAtomicAddProperty(r, mCrtc->GetId(), mCrtc->GetActivePropId(), 1);
        if (step >= 5) { /* clear every other plane too */
            for (auto &p : mPlanes) {
                if (p->GetId() == pid) continue;
                drmModeAtomicAddProperty(r, p->GetId(), p->GetPropFbId(), 0);
                drmModeAtomicAddProperty(r, p->GetId(), p->GetPropCrtcId(), 0);
            }
        }
        int ret = drmModeAtomicCommit(fd, r, DRM_MODE_ATOMIC_TEST_ONLY | DRM_MODE_ATOMIC_ALLOW_MODESET, nullptr);
        DISPLAY_LOGE("diag step %{public}d (%{public}s): plane %{public}u fb %{public}u %{public}dx%{public}d -> %{public}d errno %{public}d",
            step, what, pid, fbId, w, h, ret, ret ? errno : 0);
        drmModeAtomicFree(r);
    };
    test(0, "plane only");
    test(1, "+zpos");
    test(2, "+out fence");
    test(3, "+connector");
    test(4, "+active");
    test(5, "+clear other planes");
    for (auto &p : mPlanes) {
        DISPLAY_LOGE("diag plane %{public}u type %{public}u possible %{public}x crtc %{public}u pipe %{public}u",
            p->GetId(), p->GetType(), p->GetPossibleCrtcs(), p->GetCrtcId(), p->GetPipe());
    }
    DISPLAY_LOGE("diag crtc %{public}u conn %{public}u", mCrtc->GetId(), mConnector->GetId());
}

int32_t HdiDrmComposition::AddRelight(drmModeAtomicReqPtr pset, std::unique_ptr<DrmModeBlock> &block)
{
    int32_t id = mCrtc->GetActiveModeId();
    block = mConnector->GetModeBlockFromId(id >= 0 ? id : 0);
    if (block == nullptr || block->GetBlockId() == DRM_INVALID_ID) {
        block = mConnector->GetModeBlockFromId(0);
    }
    if (block == nullptr || block->GetBlockId() == DRM_INVALID_ID) {
        DISPLAY_LOGE("relight: no mode blob");
        return -1;
    }
    int ret = drmModeAtomicAddProperty(pset, mConnector->GetId(), mConnector->GetPropCrtcId(), mCrtc->GetId());
    ret |= drmModeAtomicAddProperty(pset, mCrtc->GetId(), mCrtc->GetModePropId(), block->GetBlockId());
    ret |= drmModeAtomicAddProperty(pset, mCrtc->GetId(), mCrtc->GetActivePropId(), 1);
    return ret < 0 ? -1 : 0;
}

bool HdiDrmComposition::AddOverlayPlane(drmModeAtomicReqPtr pset, uint32_t fb, int w, int h)
{
    int fd = mDrmDevice->GetDrmFd();
    static uint32_t planeId = 0, blendProp = 0;
    static uint64_t blendVal = 0;
    std::shared_ptr<DrmPlane> plane;
    for (auto &p : mPlanes) {
        if (planeId != 0 ? p->GetId() == planeId :
            (p->GetType() == DRM_PLANE_TYPE_OVERLAY && p->GetId() != mLastPlaneFb && p->GetPipe() == 0)) {
            plane = p;
            break;
        }
    }
    if (!plane) {
        return false;
    }
    if (planeId == 0) {
        planeId = plane->GetId();
        blendVal = HmosPlaneEnum(fd, planeId, "blend_op", "premultiplied", &blendProp);
        DISPLAY_LOGI("hmos verbose: overlay on plane %{public}u (blend_op prop %{public}u = %{public}llu)",
            planeId, blendProp, (unsigned long long)blendVal);
    }
    uint32_t pid = plane->GetId();
    drmModeAtomicAddProperty(pset, pid, plane->GetPropFbId(), fb);
    drmModeAtomicAddProperty(pset, pid, plane->GetPropCrtcId(), mCrtc->GetId());
    drmModeAtomicAddProperty(pset, pid, plane->GetPropSrc_xId(), 0);
    drmModeAtomicAddProperty(pset, pid, plane->GetPropSrc_yId(), 0);
    drmModeAtomicAddProperty(pset, pid, plane->GetPropSrc_wId(), (uint64_t)w << 16);
    drmModeAtomicAddProperty(pset, pid, plane->GetPropSrc_hId(), (uint64_t)h << 16);
    drmModeAtomicAddProperty(pset, pid, plane->GetPropCrtc_xId(), 0);
    drmModeAtomicAddProperty(pset, pid, plane->GetPropCrtc_yId(), 0);
    drmModeAtomicAddProperty(pset, pid, plane->GetPropCrtc_wId(), w);
    drmModeAtomicAddProperty(pset, pid, plane->GetPropCrtc_hId(), h);
    drmModeAtomicAddProperty(pset, pid, plane->GetPropZposId(), 1);
    if (blendProp != 0) {
        drmModeAtomicAddProperty(pset, pid, blendProp, blendVal);
    }
    return true;
}

/* screen off: scan out a black frame on our CRTC and drop every other
 * plane, so the OLED shows nothing even if some brightness write slips
 * through (the last frame was the lock screen with clock and battery).
 * The CRTC stays active: switching it off froze the panel on wake. */
void HdiDrmComposition::BlankScreen()
{
    std::lock_guard<std::mutex> lock(mApplyMutex);
    int fd = mDrmDevice->GetDrmFd();
    static uint32_t blackFb = 0;
    static int bw = 0, bh = 0;
    int w = 1080, h = 2400;
    if (!mCompLayers.empty() && mCompLayers[0]->GetCurrentBuffer() != nullptr) {
        w = mCompLayers[0]->GetCurrentBuffer()->GetWight();
        h = mCompLayers[0]->GetCurrentBuffer()->GetHeight();
    }
    if (blackFb == 0 || bw != w || bh != h) {
        struct drm_mode_create_dumb cd = {};
        cd.width = w;
        cd.height = h;
        cd.bpp = 32;
        if (ioctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &cd) != 0) {
            DISPLAY_LOGE("blank: create dumb failed errno %{public}d", errno);
            return;
        }
        struct drm_mode_map_dumb md = {};
        md.handle = cd.handle;
        if (ioctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &md) == 0) {
            void *m = mmap(nullptr, cd.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, md.offset);
            if (m != MAP_FAILED) {
                memset(m, 0, cd.size);
                munmap(m, cd.size);
            }
        }
        uint32_t handles[4] = {cd.handle}, pitches[4] = {cd.pitch}, offsets[4] = {0};
        uint32_t fb = 0;
        if (drmModeAddFB2(fd, w, h, DRM_FORMAT_XRGB8888, handles, pitches, offsets, &fb, 0) != 0) {
            DISPLAY_LOGE("blank: addfb failed errno %{public}d", errno);
            return;
        }
        blackFb = fb;
        bw = w;
        bh = h;
    }
    std::shared_ptr<DrmPlane> plane;
    for (auto &p : mPlanes) {
        if (p->GetId() == mLastPlaneFb) {
            plane = p;
        }
    }
    if (!plane && !mPrimPlanes.empty()) {
        plane = mPrimPlanes[0];
    }
    if (!plane) {
        return;
    }
    drmModeAtomicReqPtr r = drmModeAtomicAlloc();
    for (auto &p : mPlanes) {
        if (p == plane) {
            continue;
        }
        if (p->GetCrtcId() == mCrtc->GetId() || p->GetPipe() == (1u << mCrtc->GetPipe())) {
            drmModeAtomicAddProperty(r, p->GetId(), p->GetPropFbId(), 0);
            drmModeAtomicAddProperty(r, p->GetId(), p->GetPropCrtcId(), 0);
        }
    }
    uint32_t pid = plane->GetId();
    drmModeAtomicAddProperty(r, pid, plane->GetPropFbId(), blackFb);
    drmModeAtomicAddProperty(r, pid, plane->GetPropCrtcId(), mCrtc->GetId());
    drmModeAtomicAddProperty(r, pid, plane->GetPropSrc_xId(), 0);
    drmModeAtomicAddProperty(r, pid, plane->GetPropSrc_yId(), 0);
    drmModeAtomicAddProperty(r, pid, plane->GetPropSrc_wId(), (uint64_t)w << 16);
    drmModeAtomicAddProperty(r, pid, plane->GetPropSrc_hId(), (uint64_t)h << 16);
    drmModeAtomicAddProperty(r, pid, plane->GetPropCrtc_xId(), 0);
    drmModeAtomicAddProperty(r, pid, plane->GetPropCrtc_yId(), 0);
    drmModeAtomicAddProperty(r, pid, plane->GetPropCrtc_wId(), w);
    drmModeAtomicAddProperty(r, pid, plane->GetPropCrtc_hId(), h);
    drmModeAtomicAddProperty(r, pid, plane->GetPropZposId(), 0);
    int ret = drmModeAtomicCommit(fd, r, DRM_MODE_ATOMIC_ALLOW_MODESET, nullptr);
    if (ret != 0 && errno == EBUSY) {
        usleep(17000);
        ret = drmModeAtomicCommit(fd, r, DRM_MODE_ATOMIC_ALLOW_MODESET, nullptr);
    }
    drmModeAtomicFree(r);
    DISPLAY_LOGI("blank: black frame %{public}dx%{public}d on plane %{public}u ret %{public}d errno %{public}d",
        w, h, pid, ret, ret ? errno : 0);
}

/* one plane (the client layer) on our CRTC, nothing else: the exact request
 * the TEST_ONLY diagnosis accepts on the SDE driver */
int32_t HdiDrmComposition::CommitMinimal(uint64_t *outFence)
{
    if (mCompLayers.empty()) {
        return -1;
    }
    HdiDrmLayer *layer = static_cast<HdiDrmLayer *>(mCompLayers[0]);
    HdiLayerBuffer *lb = mCompLayers[0]->GetCurrentBuffer();
    DrmGemBuffer *gem = layer->GetGemBuffer();
    if (lb == nullptr || gem == nullptr || !gem->IsValid()) {
        errno = EINVAL;
        return -1;
    }
    std::shared_ptr<DrmPlane> plane;
    for (auto &p : mPlanes) {
        if (p->GetId() == mLastPlaneFb) {
            plane = p;
        }
    }
    if (!plane && !mPlanes.empty()) {
        plane = mPlanes[0];
    }
    if (!plane) {
        errno = ENODEV;
        return -1;
    }
    uint32_t fbId = g_overrideFb ? g_overrideFb : gem->GetFbId();
    int w = lb->GetWight(), h = lb->GetHeight();
    drmModeAtomicReqPtr r = drmModeAtomicAlloc();
    uint32_t pid = plane->GetId();
    drmModeAtomicAddProperty(r, pid, plane->GetPropFbId(), fbId);
    drmModeAtomicAddProperty(r, pid, plane->GetPropCrtcId(), mCrtc->GetId());
    drmModeAtomicAddProperty(r, pid, plane->GetPropSrc_xId(), 0);
    drmModeAtomicAddProperty(r, pid, plane->GetPropSrc_yId(), 0);
    drmModeAtomicAddProperty(r, pid, plane->GetPropSrc_wId(), (uint64_t)w << 16);
    drmModeAtomicAddProperty(r, pid, plane->GetPropSrc_hId(), (uint64_t)h << 16);
    drmModeAtomicAddProperty(r, pid, plane->GetPropCrtc_xId(), 0);
    drmModeAtomicAddProperty(r, pid, plane->GetPropCrtc_yId(), 0);
    drmModeAtomicAddProperty(r, pid, plane->GetPropCrtc_wId(), w);
    drmModeAtomicAddProperty(r, pid, plane->GetPropCrtc_hId(), h);
    drmModeAtomicAddProperty(r, pid, plane->GetPropZposId(), 0);
    int fence = layer->GetAcquireFenceFd();
    if (g_overrideFb == 0 && fence >= 0 && plane->GetPropFenceInId() != 0) {
        drmModeAtomicAddProperty(r, pid, plane->GetPropFenceInId(), fence);
    }
    drmModeAtomicAddProperty(r, mCrtc->GetId(), mCrtc->GetOutFencePropId(), (uint64_t)outFence);
    uint32_t flags = DRM_MODE_ATOMIC_NONBLOCK | DRM_MODE_ATOMIC_ALLOW_MODESET;
    int ret = drmModeAtomicCommit(mDrmDevice->GetDrmFd(), r, flags, nullptr);
    if (ret != 0 && errno == EINVAL) {
        std::unique_ptr<DrmModeBlock> relight;
        if (AddRelight(r, relight) == 0) {
            ret = drmModeAtomicCommit(mDrmDevice->GetDrmFd(), r, DRM_MODE_ATOMIC_ALLOW_MODESET, nullptr);
        }
    }
    if (ret != 0 && errno == EBUSY) {
        usleep(17000);
        ret = drmModeAtomicCommit(mDrmDevice->GetDrmFd(), r, flags, nullptr);
    }
    int err = errno;
    drmModeAtomicFree(r);
    errno = err;
    return ret;
}

int32_t HdiDrmComposition::Apply(bool modeSet)
{
    std::lock_guard<std::mutex> lock(mApplyMutex);
    if (HmosVerboseEnabled() && !mKeepAliveStarted) {
        mKeepAliveStarted = true;
        std::thread([this] { VerboseKeepAlive(); }).detach();
    }
    return ApplyLocked(modeSet);
}

int32_t HdiDrmComposition::ApplyLocked(bool modeSet)
{
    mLastApplyMs = HmosNowMs();
    g_overrideFb = 0;
    if (g_hmosDisplayOff.load()) {
        /* panel switched off: nothing to scan out, and a commit with an out
         * fence on an inactive CRTC is refused.  Hand back "no fence" so
         * render_service can reuse its buffers at once. */
        for (auto layer : mCompLayers) {
            layer->SetReleaseFence(-1);
        }
        return DISPLAY_SUCCESS;
    }
    /* the boot logger creates /dev/hmos_verbose_done once the system reports
     * boot completed: from then on show the plain UI */
    static bool verboseDone;
    if (!verboseDone && (access("/dev/hmos_verbose_done", F_OK) == 0 || HmosBootCompleted())) {
        verboseDone = true;
        DISPLAY_LOGI("hmos verbose: boot completed, overlay off");
    }
    uint32_t ovlFb = 0;
    if (HmosVerboseEnabled() && !verboseDone && mCompLayers.size() == 1 && mCompLayers[0]->GetCurrentBuffer()) {
        HdiLayerBuffer *cb = mCompLayers[0]->GetCurrentBuffer();
        ovlFb = HmosOverlayFb(mDrmDevice->GetDrmFd(), cb->GetWight(), cb->GetHeight());
    }
    uint64_t crtcOutFence = -1;
    int ret = 0;
    std::unique_ptr<DrmModeBlock> modeBlock;
    int drmFd = mDrmDevice->GetDrmFd();

    ret = UpdateMode(modeBlock);
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("update mode failed"));

    DISPLAY_LOGD("mPlane size: %{public}zd mCompLayers size: %{public}zd", mPlanes.size(), mCompLayers.size());
    DISPLAY_CHK_RETURN((mPlanes.size() < mCompLayers.size()), DISPLAY_FAILURE, DISPLAY_LOGE("plane not enough"));
    drmModeAtomicReqPtr pset = drmModeAtomicAlloc();
    DISPLAY_CHK_RETURN((pset == nullptr), DISPLAY_NULL_PTR,
        DISPLAY_LOGE("drm atomic alloc failed errno %{public}d", errno));
    AtomicReqPtr atomicReqPtr = AtomicReqPtr(pset);

    // set the outFence property
    ret = drmModeAtomicAddProperty(atomicReqPtr.Get(), mCrtc->GetId(), mCrtc->GetOutFencePropId(),
        (uint64_t)&crtcOutFence);

    DISPLAY_LOGD("Apply Set OutFence crtc id: %{public}d, fencePropId %{public}d", mCrtc->GetId(),
        mCrtc->GetOutFencePropId());
    DISPLAY_CHK_RETURN((ret < 0), DISPLAY_FAILURE, DISPLAY_LOGE("set the outfence property of crtc failed "));

    // set the plane info.
    DISPLAY_LOGD("mCompLayers size %{public}zd", mCompLayers.size());
    DISPLAY_LOGD("crtc id %{public}d connect id %{public}d encoder id %{public}d", mCrtc->GetId(),
        mConnector->GetId(), mConnector->GetEncoderId());

    /*  Bind the plane not used by other crtcs to the crtc. */
    FindPlaneAndApply(atomicReqPtr.Get());
    /* Remove useless planes from the drm */
    RemoveUnusePlane(atomicReqPtr.Get());
    /* verbose log plane on top (added last: libdrm keeps the last value
     * when RemoveUnusePlane already cleared this plane) */
    bool ovlUsed = false;
    if (ovlFb != 0) {
        ovlUsed = AddOverlayPlane(atomicReqPtr.Get(), ovlFb, g_shadowW, g_shadowH);
    }

    /* The CRTC is already lit (boot logger mode set / UpdateMode): plane-only
     * commits.  Re-binding the connector here makes the SDE driver reject
     * the whole commit with EINVAL. */
    /* commit strategy, learned at the first failure and kept afterwards:
     * 0 = plane-only request, 1 = same request + ALLOW_MODESET,
     * 2 = minimal single-plane request + ALLOW_MODESET (what the diag proved valid) */
    static int commitMode = 0;
    uint32_t flags = DRM_MODE_ATOMIC_NONBLOCK;
    if (commitMode == 1) {
        flags |= DRM_MODE_ATOMIC_ALLOW_MODESET;
    }
    /* pace on the previous flip instead of failing with EBUSY and sleeping a
     * whole frame (that dropped frames at 120 Hz) */
    static int prevFence = -1;
    if (prevFence >= 0) {
        struct pollfd pfd = { prevFence, POLLIN, 0 };
        poll(&pfd, 1, 25);
        close(prevFence);
        prevFence = -1;
    }
    if (commitMode == 2) {
        ret = CommitMinimal(&crtcOutFence);
    } else {
        ret = drmModeAtomicCommit(drmFd, atomicReqPtr.Get(), flags, nullptr);
        for (int i = 0; ret != 0 && errno == EBUSY && i < 12; i++) {
            usleep(1500);
            ret = drmModeAtomicCommit(drmFd, atomicReqPtr.Get(), flags, nullptr);
        }
    }
    if (ret != 0 && ovlUsed) {
        /* the second plane is refused: give up the verbose overlay for good,
         * this frame is dropped and the next one goes out without it */
        g_ovlBroken = true;
        DISPLAY_LOGE("hmos verbose: overlay plane refused (errno %{public}d), overlay disabled", errno);
        return DISPLAY_FAILURE;
    }
    if (ret != 0) {
        int err = errno;
        static int failures;
        DISPLAY_LOGE("drmModeAtomicCommit failed %{public}d errno %{public}d mode %{public}d (plane %{public}u, overlay %{public}u)",
            ret, err, commitMode, mLastPlaneFb, g_overrideFb);
        if (commitMode == 0) {
            ret = drmModeAtomicCommit(drmFd, atomicReqPtr.Get(), flags | DRM_MODE_ATOMIC_ALLOW_MODESET, nullptr);
            DISPLAY_LOGE("retry same request + ALLOW_MODESET: %{public}d errno %{public}d", ret, ret ? errno : 0);
            if (ret == 0) {
                commitMode = 1;
            }
        }
        if (ret != 0) {
            /* SDE says "requesting event but off": the CRTC is not active in
             * the atomic state (the boot logger lit it with a legacy SETCRTC or
             * it was switched off).  Light it again with a real mode set:
             * connector -> CRTC, MODE_ID blob and ACTIVE together (ACTIVE
             * without a mode is refused as "active without enabled"). */
            std::unique_ptr<DrmModeBlock> relight;
            if (AddRelight(atomicReqPtr.Get(), relight) == 0) {
                ret = drmModeAtomicCommit(drmFd, atomicReqPtr.Get(), DRM_MODE_ATOMIC_ALLOW_MODESET, nullptr);
                DISPLAY_LOGE("retry with CRTC relight (mode set): %{public}d errno %{public}d", ret, ret ? errno : 0);
                if (ret == 0) {
                    commitMode = 0; /* the CRTC is on now: plain plane commits work again */
                }
            }
        }
        if (ret != 0 && commitMode != 2) {
            crtcOutFence = -1;
            ret = CommitMinimal(&crtcOutFence);
            DISPLAY_LOGE("retry minimal request: %{public}d errno %{public}d", ret, ret ? errno : 0);
            if (ret == 0) {
                commitMode = 2;
            }
        }
        if (ret != 0) {
            static bool diagDone;
            if (!diagDone && g_overrideFb) {
                diagDone = true;
                DiagnoseCommit(g_overrideFb, g_shadowW, g_shadowH);
            }
            if (++failures % 20 == 1) {
                DISPLAY_LOGE("commit keeps failing (%{public}d times)", failures);
            }
            return DISPLAY_FAILURE;
        }
        DISPLAY_LOGI("commit ok with strategy %{public}d", commitMode);
    }
    if (static_cast<int>(crtcOutFence) >= 0) {
        prevFence = dup(static_cast<int>(crtcOutFence));
    }
    // set the release fence: every layer owns its own copy (they close it
    // when the next one arrives), the out fence itself is closed here
    int outFence = static_cast<int>(crtcOutFence);
    for (auto layer : mCompLayers) {
        layer->SetReleaseFence(outFence >= 0 ? dup(outFence) : -1);
    }
    if (outFence >= 0) {
        close(outFence);
    }

    return DISPLAY_SUCCESS;
}
} // OHOS
} // HDI
} // DISPLAY
