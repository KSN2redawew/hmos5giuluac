/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "display_composer_vdi_impl.h"
#include <hdf_base.h>
#include "display_log.h"
#include "hdf_log.h"

namespace OHOS {
namespace HDI {
namespace DISPLAY {
DisplayComposerVdiImpl::DisplayComposerVdiImpl()
{
}

DisplayComposerVdiImpl::~DisplayComposerVdiImpl()
{
}

int32_t DisplayComposerVdiImpl::RegHotPlugCallback(HotPlugCallback cb, void* data)
{
    HdiSession::GetInstance().RegHotPlugCallback(cb, data);
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::GetDisplayCapability(uint32_t devId, DisplayCapability& info)
{
    int32_t ec = HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::GetDisplayCapability, &info);
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::GetDisplaySupportedModes(uint32_t devId, std::vector<DisplayModeInfo>& modes)
{
    DisplayModeInfo* placeHoler = nullptr;
    uint32_t num = 0;
    int32_t ec = HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::GetDisplaySupportedModes,
        &num, placeHoler);
    if (ec != DISPLAY_SUCCESS) {
        DISPLAY_LOGE("failed, ec=%{public}d", ec);
        return HDF_FAILURE;
    }
    if (num != 0) {
        modes.resize(num);
        ec = HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::GetDisplaySupportedModes, &num,
            modes.data());
    }
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::GetDisplayMode(uint32_t devId, uint32_t& modeId)
{
    int32_t ec = HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::GetDisplayMode, &modeId);
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::SetDisplayMode(uint32_t devId, uint32_t modeId)
{
    int32_t ec = HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::SetDisplayMode, modeId);
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::GetDisplayPowerStatus(uint32_t devId, V1_0::DispPowerStatus& status)
{
    int32_t ec = HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::GetDisplayPowerStatus, &status);
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::SetDisplayPowerStatus(uint32_t devId, V1_0::DispPowerStatus status)
{
    int32_t ec = HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::SetDisplayPowerStatus, status);
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::GetDisplayBacklight(uint32_t devId, uint32_t& level)
{
    int32_t ec = HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::GetDisplayBacklight, &level);
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::SetDisplayBacklight(uint32_t devId, uint32_t level)
{
    int32_t ec = HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::SetDisplayBacklight, level);
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::GetDisplayProperty(uint32_t devId, uint32_t id, uint64_t& value)
{
    { static bool once; if (!once) { once = true; DISPLAY_LOGE("%s is not supported (logged once)", __func__); } }
    return HDF_ERR_NOT_SUPPORT;
}

int32_t DisplayComposerVdiImpl::GetDisplayCompChange(uint32_t devId, std::vector<uint32_t>& layers,
    std::vector<int32_t>& types)
{
    uint32_t* layersHoler = nullptr;
    int32_t* typesHoler = nullptr;
    uint32_t num = 0;
    int32_t ec = HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::GetDisplayCompChange, &num,
        layersHoler, typesHoler);
    if (ec == HDF_SUCCESS && num != 0) {
        layers.resize(num);
        types.resize(num);
        ec = HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::GetDisplayCompChange, &num,
            layers.data(), types.data());
    }
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::SetDisplayClientCrop(uint32_t devId, const IRect& rect)
{
    { static bool once; if (!once) { once = true; DISPLAY_LOGE("%s is not supported (logged once)", __func__); } }
    return HDF_ERR_NOT_SUPPORT;
}

int32_t DisplayComposerVdiImpl::SetDisplayClientBuffer(uint32_t devId, const BufferHandle& buffer, int32_t fence)
{
    int32_t ec = HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::SetDisplayClientBuffer, &buffer,
        fence);
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::SetDisplayClientDamage(uint32_t devId, std::vector<IRect>& rects)
{
    { static bool once; if (!once) { once = true; DISPLAY_LOGE("%s is not supported (logged once)", __func__); } }
    return HDF_ERR_NOT_SUPPORT;
}

int32_t DisplayComposerVdiImpl::SetDisplayVsyncEnabled(uint32_t devId, bool enabled)
{
    int32_t ec = HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::SetDisplayVsyncEnabled, enabled);
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::RegDisplayVBlankCallback(uint32_t devId, VBlankCallback cb, void* data)
{
    int32_t ec = HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::RegDisplayVBlankCallback, cb, data);
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::GetDisplayReleaseFence(uint32_t devId, std::vector<uint32_t>& layers,
    std::vector<int32_t>& fences)
{
    uint32_t* layersHoler = nullptr;
    int32_t* typesHoler = nullptr;
    uint32_t num = 0;
    int32_t ec = HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::GetDisplayReleaseFence, &num,
        layersHoler, typesHoler);
    if (ec == HDF_SUCCESS && num != 0) {
        layers.resize(num);
        fences.resize(num);
        ec = HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::GetDisplayReleaseFence, &num,
            layers.data(), fences.data());
    }
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::CreateVirtualDisplay(uint32_t width, uint32_t height, int32_t& format, uint32_t& devId)
{
    { static bool once; if (!once) { once = true; DISPLAY_LOGE("%s is not supported (logged once)", __func__); } }
    return HDF_ERR_NOT_SUPPORT;
}

int32_t DisplayComposerVdiImpl::DestroyVirtualDisplay(uint32_t devId)
{
    { static bool once; if (!once) { once = true; DISPLAY_LOGE("%s is not supported (logged once)", __func__); } }
    return HDF_ERR_NOT_SUPPORT;
}

int32_t DisplayComposerVdiImpl::SetVirtualDisplayBuffer(uint32_t devId, const BufferHandle& buffer, const int32_t fence)
{
    { static bool once; if (!once) { once = true; DISPLAY_LOGE("%s is not supported (logged once)", __func__); } }
    return HDF_ERR_NOT_SUPPORT;
}

int32_t DisplayComposerVdiImpl::SetDisplayProperty(uint32_t devId, uint32_t id, uint64_t value)
{
    { static bool once; if (!once) { once = true; DISPLAY_LOGE("%s is not supported (logged once)", __func__); } }
    return HDF_ERR_NOT_SUPPORT;
}

int32_t DisplayComposerVdiImpl::Commit(uint32_t devId, int32_t& fence)
{
    int32_t ec = HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::Commit, &fence);
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::CreateLayer(uint32_t devId, const LayerInfo& layerInfo, uint32_t& layerId)
{
    int32_t ec = HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::CreateLayer, &layerInfo, &layerId);
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::DestroyLayer(uint32_t devId, uint32_t layerId)
{
    int32_t ec = HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::DestroyLayer, layerId);
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::PrepareDisplayLayers(uint32_t devId, bool& needFlushFb)
{
    int32_t ec = HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::PrepareDisplayLayers, &needFlushFb);
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::SetLayerAlpha(uint32_t devId, uint32_t layerId, const LayerAlpha& alpha)
{
    int32_t ec = HdiSession::GetInstance().CallLayerFunction(devId, layerId, &HdiLayer::SetLayerAlpha,
        const_cast<LayerAlpha*>(&alpha));
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::SetLayerRegion(uint32_t devId, uint32_t layerId, const IRect& rect)
{
    int32_t ec = HdiSession::GetInstance().CallLayerFunction(devId, layerId, &HdiLayer::SetLayerRegion,
        const_cast<IRect*>(&rect));
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::SetLayerCrop(uint32_t devId, uint32_t layerId, const IRect& rect)
{
    int32_t ec = HdiSession::GetInstance().CallLayerFunction(devId, layerId, &HdiLayer::SetLayerCrop,
        const_cast<IRect*>(&rect));
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::SetLayerZorder(uint32_t devId, uint32_t layerId, uint32_t zorder)
{
    int32_t ec = HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::SetLayerZorder, layerId, zorder);
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::SetLayerPreMulti(uint32_t devId, uint32_t layerId, bool preMul)
{
    int32_t ec = HdiSession::GetInstance().CallLayerFunction(devId, layerId, &HdiLayer::SetLayerPreMulti, preMul);
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::SetLayerTransformMode(uint32_t devId, uint32_t layerId, TransformType type)
{
    int32_t ec = HdiSession::GetInstance().CallLayerFunction(devId, layerId, &HdiLayer::SetLayerTransformMode, type);
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::SetLayerDirtyRegion(uint32_t devId, uint32_t layerId, const std::vector<IRect>& rects)
{
    int32_t ec = HdiSession::GetInstance().CallLayerFunction(devId, layerId, &HdiLayer::SetLayerDirtyRegion,
        const_cast<IRect*>(rects.data()));
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::SetLayerVisibleRegion(uint32_t devId, uint32_t layerId, std::vector<IRect>& rects)
{
    { static bool once; if (!once) { once = true; DISPLAY_LOGE("%s is not supported (logged once)", __func__); } }
    return HDF_ERR_NOT_SUPPORT;
}

int32_t DisplayComposerVdiImpl::SetLayerBuffer(uint32_t devId, uint32_t layerId, const BufferHandle& buffer,
    int32_t fence)
{
    const BufferHandle* holder = &buffer;
    int32_t ec = HdiSession::GetInstance().CallLayerFunction(devId, layerId, &HdiLayer::SetLayerBuffer, holder, fence);
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::SetLayerCompositionType(uint32_t devId, uint32_t layerId, V1_0::CompositionType type)
{
    int32_t ec = HdiSession::GetInstance().CallLayerFunction(devId, layerId, &HdiLayer::SetLayerCompositionType, type);
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::SetLayerBlendType(uint32_t devId, uint32_t layerId, BlendType type)
{
    int32_t ec = HdiSession::GetInstance().CallLayerFunction(devId, layerId, &HdiLayer::SetLayerBlendType, type);
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::SetLayerMaskInfo(uint32_t devId, uint32_t layerId, const MaskInfo maskInfo)
{
    { static bool once; if (!once) { once = true; DISPLAY_LOGE("%s is not supported (logged once)", __func__); } }
    return HDF_ERR_NOT_SUPPORT;
}

int32_t DisplayComposerVdiImpl::SetLayerColor(uint32_t devId, uint32_t layerId, const LayerColor& layerColor)
{
    { static bool once; if (!once) { once = true; DISPLAY_LOGE("%s is not supported (logged once)", __func__); } }
    return HDF_ERR_NOT_SUPPORT;
}

/* ---------------- IDisplayComposerVdiV1_1 ---------------- */
int32_t DisplayComposerVdiImpl::RegSeamlessChangeCallback(SeamlessChangeCallback cb, void* data)
{
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::GetDisplaySupportedModesExt(uint32_t devId, std::vector<V1_1::DisplayModeInfoExt>& modes)
{
    std::vector<DisplayModeInfo> base;
    int32_t ret = GetDisplaySupportedModes(devId, base);
    if (ret != HDF_SUCCESS) {
        return ret;
    }
    modes.clear();
    for (auto &m : base) {
        V1_1::DisplayModeInfoExt e;
        e.v1_0 = m;
        e.groupId = 0;
        modes.push_back(e);
    }
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::SetDisplayModeAsync(uint32_t devId, uint32_t modeId, ModeCallback cb, void *data)
{
    int32_t ret = SetDisplayMode(devId, modeId);
    if (ret == HDF_SUCCESS && cb != nullptr) {
        cb(modeId, 0, data);
    }
    return ret;
}

int32_t DisplayComposerVdiImpl::GetDisplayVBlankPeriod(uint32_t devId, uint64_t& period)
{
    std::vector<DisplayModeInfo> modes;
    uint32_t cur = 0;
    period = 16666666; /* 60 Hz default */
    if (GetDisplaySupportedModes(devId, modes) == HDF_SUCCESS && GetDisplayMode(devId, cur) == HDF_SUCCESS) {
        for (auto &m : modes) {
            if (static_cast<uint32_t>(m.id) == cur && m.freshRate > 0) {
                period = 1000000000ULL / m.freshRate;
            }
        }
    }
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::SetLayerPerFrameParameter(uint32_t devId, uint32_t layerId, const std::string& key,
    const std::vector<int8_t>& value)
{
    return HDF_ERR_NOT_SUPPORT;
}

int32_t DisplayComposerVdiImpl::GetSupportedLayerPerFrameParameterKey(std::vector<std::string>& keys)
{
    keys.clear();
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::SetDisplayOverlayResolution(uint32_t devId, uint32_t width, uint32_t height)
{
    return HDF_ERR_NOT_SUPPORT;
}

int32_t DisplayComposerVdiImpl::RegRefreshCallback(RefreshCallback cb, void* data)
{
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::GetDisplaySupportedColorGamuts(uint32_t devId, std::vector<ColorGamut>& gamuts)
{
    gamuts.clear();
    gamuts.push_back(COLOR_GAMUT_SRGB);
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::GetHDRCapabilityInfos(uint32_t devId, HDRCapability& info)
{
    info.formatCount = 0;
    info.formats.clear();
    info.maxLum = 0;
    info.maxAverageLum = 0;
    info.minLum = 0;
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::RegDisplayVBlankIdleCallback(VBlankIdleCallback cb, void* data)
{
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::SetDisplayConstraint(uint32_t devId, uint64_t frameID, uint64_t ns, uint32_t type)
{
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::SetHardwareCursorPosition(uint32_t devId, int32_t x, int32_t y)
{
    return HDF_ERR_NOT_SUPPORT;
}

int32_t DisplayComposerVdiImpl::EnableHardwareCursorStats(uint32_t devId, bool enable)
{
    return HDF_ERR_NOT_SUPPORT;
}

int32_t DisplayComposerVdiImpl::GetHardwareCursorStats(uint32_t devId, uint32_t& frameCount, uint32_t& vsyncCount)
{
    return HDF_ERR_NOT_SUPPORT;
}

int32_t DisplayComposerVdiImpl::SetDisplayActiveRegion(uint32_t devId, const IRect& rect)
{
    return HDF_ERR_NOT_SUPPORT;
}

extern "C" IDisplayComposerVdiV1_1 *CreateComposerVdiV1_1()
{
    return new DisplayComposerVdiImpl();
}

extern "C" void DestroyComposerVdiV1_1(IDisplayComposerVdiV1_1* vdi)
{
    delete vdi;
}

extern "C" IDisplayComposerVdi *CreateComposerVdi()
{
    return new DisplayComposerVdiImpl();
}

extern "C" void DestroyComposerVdi(IDisplayComposerVdi* vdi)
{
    delete vdi;
}
} // DISPLAY
} // HDI
} // OHOS
