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

#include <cstdarg>
#include "drm_display.h"
#include <string>
#include <cerrno>
#include <memory>
#include <sys/ioctl.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include "idisplay_buffer_vdi.h"
using OHOS::HDI::Display::Buffer::V1_0::IDisplayBufferVdi;
extern "C" IDisplayBufferVdi* CreateDisplayBufferVdi();
#include "display_log.h"
#include "drm_device.h"
#include "drm_vsync_worker.h"
#include "hdi_drm_composition.h"
#include "hdi_gfx_composition.h"
#include "idisplay_buffer_vdi.h"

#include <atomic>
namespace OHOS {
namespace HDI {
namespace DISPLAY {
extern std::atomic<bool> g_hmosDisplayOff;
/* display power events also go to the kernel log: hilog flow control
 * drops composer lines when the system is busy */
static void HmosKlog(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
static void HmosKlog(const char *fmt, ...)
{
    char buf[200];
    int n = snprintf(buf, sizeof(buf), "<14>");
    va_list ap;
    va_start(ap, fmt);
    n += vsnprintf(buf + n, sizeof(buf) - n - 1, fmt, ap);
    va_end(ap);
    if (n > static_cast<int>(sizeof(buf)) - 2) {
        n = sizeof(buf) - 2;
    }
    buf[n++] = '\n';
    int fd = open("/dev/kmsg", O_WRONLY | O_CLOEXEC);
    if (fd >= 0) {
        write(fd, buf, n);
        close(fd);
    }
    DISPLAY_LOGI("%{public}s", buf + 4);
}
static uint32_t g_hmosSavedLevel = 0;
extern std::atomic<uint64_t> g_op13VsyncPeriodNs;
}
}

namespace HDI {
namespace DISPLAY {
using namespace OHOS::HDI::Display::Buffer::V1_0;
DrmDisplay::DrmDisplay(std::shared_ptr<DrmConnector> connector, std::shared_ptr<DrmCrtc> crtc,
    std::shared_ptr<DrmDevice> drmDevice)
    : mDrmDevice(drmDevice), mConnector(connector), mCrtc(crtc)
{}

DrmDisplay::~DrmDisplay()
{
    if (mCrtc != nullptr) {
        mCrtc->UnBindDisplay(GetId());
    }
}

int32_t DrmDisplay::Init()
{
    int ret;
    DISPLAY_CHK_RETURN((mCrtc == nullptr), DISPLAY_FAILURE, DISPLAY_LOGE("crtc is null"));
    DISPLAY_CHK_RETURN((mConnector == nullptr), DISPLAY_FAILURE, DISPLAY_LOGE("connector is null"));
    DISPLAY_CHK_RETURN((mDrmDevice == nullptr), DISPLAY_FAILURE, DISPLAY_LOGE("drmDevice is null"));

    ret = HdiDisplay::Init();
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("init failed"));
    auto preComp = std::make_unique<HdiGfxComposition>();
    DISPLAY_CHK_RETURN((preComp == nullptr), DISPLAY_FAILURE,
        DISPLAY_LOGE("can not new HdiGfxComposition errno %{public}d", errno));
    ret = preComp->Init();
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("can not init HdiGfxComposition"));

    auto postComp = std::make_unique<HdiDrmComposition>(mConnector, mCrtc, mDrmDevice);
    DISPLAY_CHK_RETURN((postComp == nullptr), DISPLAY_FAILURE,
        DISPLAY_LOGE("can not new HdiDrmComposition errno %{public}d", errno));
    ret = postComp->Init();
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("can not init HdiDrmComposition"));
    mComposer = std::make_unique<HdiComposer>(std::move(preComp), std::move(postComp));
    ret = mCrtc->BindToDisplay(GetId());
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("bind crtc failed"));

    ret = ChosePreferenceMode();
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("choose preference mode fialed"));
    return DISPLAY_SUCCESS;
}

int32_t DrmDisplay::GetDisplayCapability(DisplayCapability *info)
{
    mConnector->GetDisplayCap(*info);
    return DISPLAY_SUCCESS;
}

/* OP13 exposes real DRM modes. Mode IDs are connector IDs, never a
 * synthetic 60 Hz selector. Keep the timing model in step with the mode
 * queued for the next atomic commit. */
int32_t DrmDisplay::GetDisplaySupportedModes(uint32_t *num, DisplayModeInfo *modes)
{
    return mConnector->GetDisplaySupportedModes(num, modes);
}

int32_t DrmDisplay::GetDisplayMode(uint32_t *modeId)
{
    DISPLAY_CHK_RETURN((modeId == nullptr), DISPLAY_NULL_PTR, DISPLAY_LOGE("modeId is null"));
    int32_t active = mCrtc->GetActiveModeId();
    DISPLAY_CHK_RETURN((active < 0), DISPLAY_FAILURE, DISPLAY_LOGE("no active mode"));
    *modeId = static_cast<uint32_t>(active);
    return DISPLAY_SUCCESS;
}

int32_t DrmDisplay::SetDisplayMode(uint32_t modeId)
{
    DISPLAY_CHK_RETURN((modeId > INT32_MAX), DISPLAY_PARAM_ERR, DISPLAY_LOGE("invalid mode"));
    DrmMode mode;
    int32_t ret = mConnector->GetModeFromId(static_cast<int32_t>(modeId), mode);
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_PARAM_ERR, DISPLAY_LOGE("unknown mode"));
    auto *timing = mode.GetModeInfoPtr();
    uint64_t rate = timing->vrefresh;
    if (!rate && timing->htotal && timing->vtotal) {
        rate = (static_cast<uint64_t>(timing->clock) * 1000ULL) /
            (static_cast<uint64_t>(timing->htotal) * timing->vtotal);
    }
    DISPLAY_CHK_RETURN((rate == 0 || rate > 1000), DISPLAY_PARAM_ERR, DISPLAY_LOGE("invalid refresh"));
    ret = mCrtc->SetActivieMode(static_cast<int32_t>(modeId));
    if (ret == DISPLAY_SUCCESS) {
        g_op13VsyncPeriodNs.store((1000000000ULL + rate / 2) / rate);
        DISPLAY_LOGI("OP13 mode %{public}u at %{public}llu Hz", modeId,
            static_cast<unsigned long long>(rate));
    }
    return ret;
}

int32_t DrmDisplay::GetDisplayPowerStatus(DispPowerStatus *status)
{
    DISPLAY_CHK_RETURN((status == nullptr), DISPLAY_NULL_PTR, DISPLAY_LOGE("status is nullptr"));
    *status = mPowerStatus;
    return DISPLAY_SUCCESS;
}

int32_t DrmDisplay::SetDisplayPowerStatus(DispPowerStatus status)
{
    static int32_t fb_fd  = -1;
    int32_t err = -1;
    const uint32_t FBIOBLANK = 0x4611; /* arg: 0 or vesa level + 1 */
    const uint32_t FB_BLANK_UNBLANK = 0;  /* screen: unblanked, hsync: on,  vsync: on */
    const uint32_t FB_BLANK_POWERDOWN = 4; /* screen: blanked,   hsync: off, vsync: off */

    DISPLAY_LOGD("SetDisplayPowerStatus power state %{public}u", status);
    if (fb_fd < 0) {
        fb_fd = open("/dev/graphics/fb0", O_RDWR);
        if (fb_fd < 0) {
            /* msm drm (sweet) has no fbdev node. Switching the CRTC off and on
             * again from here left the panel lit but frozen after the first
             * screen-off (the CRTC/plane state did not come back). Keep the
             * pipeline running and only blank the OLED: backlight 0, no
             * commits, quiet vsync. When the system suspends, the msm DRM
             * driver powers the panel down itself and restores it on resume. */
            /* 4 = OFF_FAKE, 5 = PRE_ON, 6 = PRE_OFF (display composer 1.2) */
            uint32_t st = static_cast<uint32_t>(status);
            bool off = (status == POWER_STATUS_OFF || status == POWER_STATUS_STANDBY ||
                status == POWER_STATUS_SUSPEND || st == 4);
            HmosKlog("composer: SetDisplayPowerStatus %u (was %u, panel %s)", st,
                static_cast<uint32_t>(mPowerStatus), g_hmosDisplayOff.load() ? "off" : "on");
            if (!off && status != POWER_STATUS_ON) {
                /* PRE_ON / PRE_OFF and unknown hints: keep the current state */
                return DISPLAY_SUCCESS;
            }
            if (off) {
                uint32_t cur = 0;
                mConnector->GetBrightness(cur);
                if (cur != 0 && !g_hmosDisplayOff.load()) {
                    g_hmosSavedLevel = cur;
                }
                if (GetDrmComposition() != nullptr) {
                    GetDrmComposition()->BlankScreen();
                }
                g_hmosDisplayOff.store(true);
                mConnector->SetBrightness(0);
            } else {
                g_hmosDisplayOff.store(false);
                uint32_t cur = 0;
                mConnector->GetBrightness(cur);
                if (cur == 0 && g_hmosSavedLevel != 0) {
                    mConnector->SetBrightness(g_hmosSavedLevel);
                }
            }
            mPowerStatus = status;
            DISPLAY_LOGI("power state %{public}u (panel %{public}s)", status, off ? "blanked" : "on");
            return DISPLAY_SUCCESS;
        }
    }
    switch (status) {
        case POWER_STATUS_OFF:
        case POWER_STATUS_STANDBY:
        case POWER_STATUS_SUSPEND:
            err = ioctl(fb_fd, FBIOBLANK, FB_BLANK_POWERDOWN);
            break;
        case POWER_STATUS_ON:
            err = ioctl(fb_fd, FBIOBLANK, FB_BLANK_UNBLANK);
            break;
        default:
            err = DISPLAY_FAILURE;
            break;
    }
    if (err < 0) {
        DISPLAY_LOGE("ioctl fb0 failed\n");
        return DISPLAY_FAILURE;
    }

    return DISPLAY_SUCCESS;
}

int32_t DrmDisplay::ConvertToHdiPowerState(uint32_t drmPowerState, DispPowerStatus &hdiPowerState)
{
    int32_t ret = DISPLAY_SUCCESS;
    switch (drmPowerState) {
        case DRM_MODE_DPMS_OFF:
            hdiPowerState = POWER_STATUS_OFF;
            break;
        case DRM_MODE_DPMS_ON:
            hdiPowerState = POWER_STATUS_ON;
            break;
        case DRM_MODE_DPMS_STANDBY:
            hdiPowerState = POWER_STATUS_STANDBY;
            break;
        case DRM_MODE_DPMS_SUSPEND:
            hdiPowerState = POWER_STATUS_SUSPEND;
            break;
        default:
            hdiPowerState = POWER_STATUS_BUTT;
            ret = DISPLAY_FAILURE;
            break;
    }
    DISPLAY_LOGD("hdi power state %{public}u", hdiPowerState);
    return ret;
}

int32_t DrmDisplay::ConvertToDrmPowerState(DispPowerStatus hdiPowerState, uint32_t &drmPowerState)
{
    int32_t ret = DISPLAY_SUCCESS;
    switch (hdiPowerState) {
        case POWER_STATUS_OFF:
            drmPowerState = DRM_MODE_DPMS_OFF;
            break;
        case POWER_STATUS_ON:
            drmPowerState = DRM_MODE_DPMS_ON;
            break;
        case POWER_STATUS_STANDBY:
            drmPowerState = DRM_MODE_DPMS_STANDBY;
            break;
        case POWER_STATUS_SUSPEND:
            drmPowerState = DRM_MODE_DPMS_SUSPEND;
            break;
        default:
            ret = DISPLAY_FAILURE;
            break;
    }
    return ret;
}

std::unique_ptr<HdiLayer> DrmDisplay::CreateHdiLayer(LayerType type)
{
    DISPLAY_LOGD();
    return std::make_unique<HdiDrmLayer>(type);
}

int32_t DrmDisplay::WaitForVBlank(uint64_t *ns)
{
    int ret;
    constexpr uint64_t nPerS = 1000000000;
    constexpr uint64_t nPerUS = 1000;
    drmVBlank vbl = {
        .request.type = DRM_VBLANK_RELATIVE,
        .request.sequence = 0,
        .request.signal = 0,
    };
    DISPLAY_CHK_RETURN((ns == nullptr), DISPLAY_NULL_PTR, DISPLAY_LOGE("in ns is nullptr"));
    ret = drmWaitVBlank(mDrmDevice->GetDrmFd(), &vbl);
    DISPLAY_CHK_RETURN((ret != 0), DISPLAY_FAILURE, DISPLAY_LOGE("wait vblank failed errno %{public}d", errno));
    *ns = static_cast<uint64_t>(vbl.reply.tval_sec * nPerS + vbl.reply.tval_usec * nPerUS);
    return DISPLAY_SUCCESS;
}

bool DrmDisplay::IsConnected()
{
    DISPLAY_LOGD("conneted %{public}d", mConnector->IsConnected());
    return mConnector->IsConnected();
}

int32_t DrmDisplay::PushFirstFrame()
{
    std::shared_ptr<IDisplayBufferVdi> hdiImpl(CreateDisplayBufferVdi());
    int ret = DISPLAY_SUCCESS;
    DrmMode mode;
    ret = mConnector->GetModeFromId(mCrtc->GetActiveModeId(), mode);
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE,
        DISPLAY_LOGE("can not get the mode from id %{public}d", mCrtc->GetActiveModeId()));
    const AllocInfo info = {
        .width = mode.GetModeInfoPtr()->hdisplay,
        .height = mode.GetModeInfoPtr()->vdisplay,
        .usage = HBM_USE_MEM_DMA | HBM_USE_CPU_READ | HBM_USE_CPU_WRITE,
        .format = PIXEL_FMT_BGRA_8888
    };

    BufferHandle *buffer = nullptr;
    ret = hdiImpl->AllocMem(info, buffer);
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("can not alloc memory"));
    mClientLayer->SetLayerBuffer(buffer, -1);

    std::vector<HdiLayer *> layers;
    HdiDrmComposition *drmComp = static_cast<HdiDrmComposition *>(mComposer->GetPostCompostion());
    drmComp->SetLayers(layers, *mClientLayer);
    drmComp->Apply(true);
    return DISPLAY_SUCCESS;
}

HdiDrmComposition *DrmDisplay::GetDrmComposition()
{
    if (mComposer == nullptr) {
        return nullptr;
    }
    return static_cast<HdiDrmComposition *>(mComposer->GetPostCompostion());
}

int32_t DrmDisplay::ChosePreferenceMode()
{
    int32_t ret;
    int32_t modeId = mConnector->GetPreferenceId();
    if (modeId == INVALID_MODE_ID) {
        uint32_t num = 0;
        ret = GetDisplaySupportedModes(&num, nullptr);
        DISPLAY_CHK_RETURN((num == 0) || (ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("can not get modes"));
        modeId = 0;
    }
    ret = SetDisplayMode(modeId);
    // Push first frame to the drm, for that the vblank must init all the componet.
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("set display mode failed"));
    return PushFirstFrame();
}

int32_t DrmDisplay::RegDisplayVBlankCallback(VBlankCallback cb, void *data)
{
    (void)data;
    std::shared_ptr<VsyncCallBack> vsyncCb = std::make_shared<VsyncCallBack>(cb, data, mCrtc->GetPipe());
    DrmVsyncWorker::GetInstance().ReqesterVBlankCb(vsyncCb);
    return DISPLAY_SUCCESS;
}

int32_t DrmDisplay::SetDisplayVsyncEnabled(bool enabled)
{
    DISPLAY_LOGD("enable %{public}d", enabled);
    DrmVsyncWorker::GetInstance().EnableVsync(enabled);
    return DISPLAY_SUCCESS;
}

int32_t DrmDisplay::GetDisplayBacklight(uint32_t *value)
{
    DISPLAY_CHK_RETURN((value == nullptr), DISPLAY_NULL_PTR, DISPLAY_LOGE("value is nullptr"));
    return mConnector->GetBrightness(*value);
}

int32_t DrmDisplay::SetDisplayBacklight(uint32_t value)
{
    if (g_hmosDisplayOff.load()) {
        /* panel is off: powermgr may still animate the brightness (dim /
         * off transition, light sensor). Remember the level for the wake-up
         * but keep the OLED dark. */
        if (value != 0) {
            g_hmosSavedLevel = value;
        }
        HmosKlog("composer: backlight %u held (panel off)", value);
        return DISPLAY_SUCCESS;
    }
    static uint32_t lastLogged = 1;
    if ((value == 0) != (lastLogged == 0)) {
        HmosKlog("composer: backlight %u", value);
    }
    lastLogged = value;
    return mConnector->SetBrightness(value);
}
} // namespace OHOS
} // namespace HDI
} // namespace DISPLAY
