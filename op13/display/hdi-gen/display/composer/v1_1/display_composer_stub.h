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

#ifndef OHOS_HDI_DISPLAY_COMPOSER_V1_1_DISPLAYCOMPOSERSTUB_H
#define OHOS_HDI_DISPLAY_COMPOSER_V1_1_DISPLAYCOMPOSERSTUB_H

#include <ipc_object_stub.h>
#include <message_option.h>
#include <message_parcel.h>
#include <object_collector.h>
#include <refbase.h>
#include "v1_0/display_composer_stub.h"
#include "v1_1/idisplay_composer.h"

namespace OHOS {
namespace HDI {
namespace Display {
namespace Composer {
namespace V1_1 {

using namespace OHOS;
class DisplayComposerStub : public IPCObjectStub {
public:
    explicit DisplayComposerStub(const sptr<IDisplayComposer> &impl);
    virtual ~DisplayComposerStub();

    int32_t OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;

    static int32_t DisplayComposerStubRegSeamlessChangeCallback_(MessageParcel& displayComposerData, MessageParcel& displayComposerReply, MessageOption& displayComposerOption, sptr<OHOS::HDI::Display::Composer::V1_1::IDisplayComposer> impl);

    static int32_t DisplayComposerStubGetDisplaySupportedModesExt_(MessageParcel& displayComposerData, MessageParcel& displayComposerReply, MessageOption& displayComposerOption, sptr<OHOS::HDI::Display::Composer::V1_1::IDisplayComposer> impl);

    static int32_t DisplayComposerStubSetDisplayModeAsync_(MessageParcel& displayComposerData, MessageParcel& displayComposerReply, MessageOption& displayComposerOption, sptr<OHOS::HDI::Display::Composer::V1_1::IDisplayComposer> impl);

    static int32_t DisplayComposerStubGetDisplayVBlankPeriod_(MessageParcel& displayComposerData, MessageParcel& displayComposerReply, MessageOption& displayComposerOption, sptr<OHOS::HDI::Display::Composer::V1_1::IDisplayComposer> impl);

    static int32_t DisplayComposerStubSetLayerPerFrameParameter_(MessageParcel& displayComposerData, MessageParcel& displayComposerReply, MessageOption& displayComposerOption, sptr<OHOS::HDI::Display::Composer::V1_1::IDisplayComposer> impl);

    static int32_t DisplayComposerStubGetSupportedLayerPerFrameParameterKey_(MessageParcel& displayComposerData, MessageParcel& displayComposerReply, MessageOption& displayComposerOption, sptr<OHOS::HDI::Display::Composer::V1_1::IDisplayComposer> impl);

    static int32_t DisplayComposerStubSetDisplayOverlayResolution_(MessageParcel& displayComposerData, MessageParcel& displayComposerReply, MessageOption& displayComposerOption, sptr<OHOS::HDI::Display::Composer::V1_1::IDisplayComposer> impl);

    static int32_t DisplayComposerStubRegRefreshCallback_(MessageParcel& displayComposerData, MessageParcel& displayComposerReply, MessageOption& displayComposerOption, sptr<OHOS::HDI::Display::Composer::V1_1::IDisplayComposer> impl);

    static int32_t DisplayComposerStubGetDisplaySupportedColorGamuts_(MessageParcel& displayComposerData, MessageParcel& displayComposerReply, MessageOption& displayComposerOption, sptr<OHOS::HDI::Display::Composer::V1_1::IDisplayComposer> impl);

    static int32_t DisplayComposerStubGetHDRCapabilityInfos_(MessageParcel& displayComposerData, MessageParcel& displayComposerReply, MessageOption& displayComposerOption, sptr<OHOS::HDI::Display::Composer::V1_1::IDisplayComposer> impl);

private:
    int32_t DisplayComposerStubRegSeamlessChangeCallback(MessageParcel& displayComposerData, MessageParcel& displayComposerReply, MessageOption& displayComposerOption);

    int32_t DisplayComposerStubGetDisplaySupportedModesExt(MessageParcel& displayComposerData, MessageParcel& displayComposerReply, MessageOption& displayComposerOption);

    int32_t DisplayComposerStubSetDisplayModeAsync(MessageParcel& displayComposerData, MessageParcel& displayComposerReply, MessageOption& displayComposerOption);

    int32_t DisplayComposerStubGetDisplayVBlankPeriod(MessageParcel& displayComposerData, MessageParcel& displayComposerReply, MessageOption& displayComposerOption);

    int32_t DisplayComposerStubSetLayerPerFrameParameter(MessageParcel& displayComposerData, MessageParcel& displayComposerReply, MessageOption& displayComposerOption);

    int32_t DisplayComposerStubGetSupportedLayerPerFrameParameterKey(MessageParcel& displayComposerData, MessageParcel& displayComposerReply, MessageOption& displayComposerOption);

    int32_t DisplayComposerStubSetDisplayOverlayResolution(MessageParcel& displayComposerData, MessageParcel& displayComposerReply, MessageOption& displayComposerOption);

    int32_t DisplayComposerStubRegRefreshCallback(MessageParcel& displayComposerData, MessageParcel& displayComposerReply, MessageOption& displayComposerOption);

    int32_t DisplayComposerStubGetDisplaySupportedColorGamuts(MessageParcel& displayComposerData, MessageParcel& displayComposerReply, MessageOption& displayComposerOption);

    int32_t DisplayComposerStubGetHDRCapabilityInfos(MessageParcel& displayComposerData, MessageParcel& displayComposerReply, MessageOption& displayComposerOption);

    int32_t DisplayComposerStubRegHotPlugCallback(MessageParcel& displayComposerData, MessageParcel& displayComposerReply, MessageOption& displayComposerOption);

    int32_t DisplayComposerStubSetClientBufferCacheCount(MessageParcel& displayComposerData, MessageParcel& displayComposerReply, MessageOption& displayComposerOption);

    int32_t DisplayComposerStubRegDisplayVBlankCallback(MessageParcel& displayComposerData, MessageParcel& displayComposerReply, MessageOption& displayComposerOption);

    int32_t DisplayComposerStubGetDisplayCapability(MessageParcel& displayComposerData, MessageParcel& displayComposerReply, MessageOption& displayComposerOption);

    int32_t DisplayComposerStubGetDisplaySupportedModes(MessageParcel& displayComposerData, MessageParcel& displayComposerReply, MessageOption& displayComposerOption);

    int32_t DisplayComposerStubGetDisplayMode(MessageParcel& displayComposerData, MessageParcel& displayComposerReply, MessageOption& displayComposerOption);

    int32_t DisplayComposerStubSetDisplayMode(MessageParcel& displayComposerData, MessageParcel& displayComposerReply, MessageOption& displayComposerOption);

    int32_t DisplayComposerStubGetDisplayPowerStatus(MessageParcel& displayComposerData, MessageParcel& displayComposerReply, MessageOption& displayComposerOption);

    int32_t DisplayComposerStubSetDisplayPowerStatus(MessageParcel& displayComposerData, MessageParcel& displayComposerReply, MessageOption& displayComposerOption);

    int32_t DisplayComposerStubGetDisplayBacklight(MessageParcel& displayComposerData, MessageParcel& displayComposerReply, MessageOption& displayComposerOption);

    int32_t DisplayComposerStubSetDisplayBacklight(MessageParcel& displayComposerData, MessageParcel& displayComposerReply, MessageOption& displayComposerOption);

    int32_t DisplayComposerStubSetDisplayVsyncEnabled(MessageParcel& displayComposerData, MessageParcel& displayComposerReply, MessageOption& displayComposerOption);

    int32_t DisplayComposerStubCreateLayer(MessageParcel& displayComposerData, MessageParcel& displayComposerReply, MessageOption& displayComposerOption);

    int32_t DisplayComposerStubDestroyLayer(MessageParcel& displayComposerData, MessageParcel& displayComposerReply, MessageOption& displayComposerOption);

    int32_t DisplayComposerStubSetDisplayClientCrop(MessageParcel& displayComposerData, MessageParcel& displayComposerReply, MessageOption& displayComposerOption);

    int32_t DisplayComposerStubGetDisplayReleaseFence(MessageParcel& displayComposerData, MessageParcel& displayComposerReply, MessageOption& displayComposerOption);

    int32_t DisplayComposerStubCreateVirtualDisplay(MessageParcel& displayComposerData, MessageParcel& displayComposerReply, MessageOption& displayComposerOption);

    int32_t DisplayComposerStubDestroyVirtualDisplay(MessageParcel& displayComposerData, MessageParcel& displayComposerReply, MessageOption& displayComposerOption);

    int32_t DisplayComposerStubSetVirtualDisplayBuffer(MessageParcel& displayComposerData, MessageParcel& displayComposerReply, MessageOption& displayComposerOption);

    int32_t DisplayComposerStubSetDisplayProperty(MessageParcel& displayComposerData, MessageParcel& displayComposerReply, MessageOption& displayComposerOption);

    int32_t DisplayComposerStubGetDisplayProperty(MessageParcel& displayComposerData, MessageParcel& displayComposerReply, MessageOption& displayComposerOption);

    int32_t DisplayComposerStubInitCmdRequest(MessageParcel& displayComposerData, MessageParcel& displayComposerReply, MessageOption& displayComposerOption);

    int32_t DisplayComposerStubCmdRequest(MessageParcel& displayComposerData, MessageParcel& displayComposerReply, MessageOption& displayComposerOption);

    int32_t DisplayComposerStubGetCmdReply(MessageParcel& displayComposerData, MessageParcel& displayComposerReply, MessageOption& displayComposerOption);

    int32_t DisplayComposerStubGetVersion(MessageParcel& displayComposerData, MessageParcel& displayComposerReply, MessageOption& displayComposerOption);


    static inline ObjectDelegator<OHOS::HDI::Display::Composer::V1_1::DisplayComposerStub, OHOS::HDI::Display::Composer::V1_1::IDisplayComposer> objDelegator_;
    sptr<OHOS::HDI::Display::Composer::V1_1::IDisplayComposer> impl_;
};
} // V1_1
} // Composer
} // Display
} // HDI
} // OHOS

#endif // OHOS_HDI_DISPLAY_COMPOSER_V1_1_DISPLAYCOMPOSERSTUB_H