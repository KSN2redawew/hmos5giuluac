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

#ifndef OHOS_HDI_DISPLAY_COMPOSER_V1_1_DISPLAYCOMPOSERSERVICE_H
#define OHOS_HDI_DISPLAY_COMPOSER_V1_1_DISPLAYCOMPOSERSERVICE_H

#include "v1_1/idisplay_composer.h"

namespace OHOS {
namespace HDI {
namespace Display {
namespace Composer {
namespace V1_1 {
class DisplayComposerService : public OHOS::HDI::Display::Composer::V1_1::IDisplayComposer {
public:
    DisplayComposerService() = default;
    virtual ~DisplayComposerService() = default;

    int32_t RegSeamlessChangeCallback(const sptr<OHOS::HDI::Display::Composer::V1_1::ISeamlessChangeCallback>& cb) override;

    int32_t GetDisplaySupportedModesExt(uint32_t devId,
         std::vector<OHOS::HDI::Display::Composer::V1_1::DisplayModeInfoExt>& modes) override;

    int32_t SetDisplayModeAsync(uint32_t devId, uint32_t modeId,
         const sptr<OHOS::HDI::Display::Composer::V1_1::IModeCallback>& cb) override;

    int32_t GetDisplayVBlankPeriod(uint32_t devId, uint64_t& period) override;

    int32_t SetLayerPerFrameParameter(uint32_t devId, uint32_t layerId, const std::string& key,
         const std::vector<int8_t>& value) override;

    int32_t GetSupportedLayerPerFrameParameterKey(std::vector<std::string>& keys) override;

    int32_t SetDisplayOverlayResolution(uint32_t devId, uint32_t width, uint32_t height) override;

    int32_t RegRefreshCallback(const sptr<OHOS::HDI::Display::Composer::V1_0::IRefreshCallback>& cb) override;

    int32_t GetDisplaySupportedColorGamuts(uint32_t devId,
         std::vector<OHOS::HDI::Display::Composer::V1_0::ColorGamut>& gamuts) override;

    int32_t GetHDRCapabilityInfos(uint32_t devId, OHOS::HDI::Display::Composer::V1_0::HDRCapability& info) override;

    int32_t RegHotPlugCallback(const sptr<OHOS::HDI::Display::Composer::V1_0::IHotPlugCallback>& cb) override;

    int32_t SetClientBufferCacheCount(uint32_t devId, uint32_t count) override;

    int32_t RegDisplayVBlankCallback(uint32_t devId,
         const sptr<OHOS::HDI::Display::Composer::V1_0::IVBlankCallback>& cb) override;

    int32_t GetDisplayCapability(uint32_t devId, OHOS::HDI::Display::Composer::V1_0::DisplayCapability& info) override;

    int32_t GetDisplaySupportedModes(uint32_t devId,
         std::vector<OHOS::HDI::Display::Composer::V1_0::DisplayModeInfo>& modes) override;

    int32_t GetDisplayMode(uint32_t devId, uint32_t& modeId) override;

    int32_t SetDisplayMode(uint32_t devId, uint32_t modeId) override;

    int32_t GetDisplayPowerStatus(uint32_t devId, OHOS::HDI::Display::Composer::V1_0::DispPowerStatus& status) override;

    int32_t SetDisplayPowerStatus(uint32_t devId, OHOS::HDI::Display::Composer::V1_0::DispPowerStatus status) override;

    int32_t GetDisplayBacklight(uint32_t devId, uint32_t& level) override;

    int32_t SetDisplayBacklight(uint32_t devId, uint32_t level) override;

    int32_t SetDisplayVsyncEnabled(uint32_t devId, bool enabled) override;

    int32_t CreateLayer(uint32_t devId, const OHOS::HDI::Display::Composer::V1_0::LayerInfo& layerInfo,
         uint32_t cacheCount, uint32_t& layerId) override;

    int32_t DestroyLayer(uint32_t devId, uint32_t layerId) override;

    int32_t SetDisplayClientCrop(uint32_t devId, const OHOS::HDI::Display::Composer::V1_0::IRect& rect) override;

    int32_t GetDisplayReleaseFence(uint32_t devId, std::vector<uint32_t>& layers,
         std::vector<sptr<OHOS::HDI::Display::HdifdParcelable>>& fences) override;

    int32_t CreateVirtualDisplay(uint32_t width, uint32_t height, int32_t& format, uint32_t& devId) override;

    int32_t DestroyVirtualDisplay(uint32_t devId) override;

    int32_t SetVirtualDisplayBuffer(uint32_t devId, const sptr<NativeBuffer>& buffer,
         const sptr<OHOS::HDI::Display::HdifdParcelable>& fence) override;

    int32_t SetDisplayProperty(uint32_t devId, uint32_t id, uint64_t value) override;

    int32_t GetDisplayProperty(uint32_t devId, uint32_t id, uint64_t& value) override;

    int32_t InitCmdRequest(const std::shared_ptr<SharedMemQueue<int32_t>>& request) override;

    int32_t CmdRequest(uint32_t inEleCnt, const std::vector<OHOS::HDI::Display::Composer::V1_0::HdifdInfo>& inFds,
         uint32_t& outEleCnt, std::vector<OHOS::HDI::Display::Composer::V1_0::HdifdInfo>& outFds) override;

    int32_t GetCmdReply(std::shared_ptr<SharedMemQueue<int32_t>>& reply) override;

};
} // V1_1
} // Composer
} // Display
} // HDI
} // OHOS

#endif // OHOS_HDI_DISPLAY_COMPOSER_V1_1_DISPLAYCOMPOSERSERVICE_H