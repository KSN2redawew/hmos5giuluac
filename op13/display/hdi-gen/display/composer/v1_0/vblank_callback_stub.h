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

#ifndef OHOS_HDI_DISPLAY_COMPOSER_V1_0_VBLANKCALLBACKSTUB_H
#define OHOS_HDI_DISPLAY_COMPOSER_V1_0_VBLANKCALLBACKSTUB_H

#include <ipc_object_stub.h>
#include <message_option.h>
#include <message_parcel.h>
#include <object_collector.h>
#include <refbase.h>
#include "v1_0/iv_blank_callback.h"

namespace OHOS {
namespace HDI {
namespace Display {
namespace Composer {
namespace V1_0 {

using namespace OHOS;
class VBlankCallbackStub : public IPCObjectStub {
public:
    explicit VBlankCallbackStub(const sptr<IVBlankCallback> &impl);
    virtual ~VBlankCallbackStub();

    int32_t OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;

    static int32_t VBlankCallbackStubOnVBlank_(MessageParcel& vBlankCallbackData, MessageParcel& vBlankCallbackReply, MessageOption& vBlankCallbackOption, sptr<OHOS::HDI::Display::Composer::V1_0::IVBlankCallback> impl);

    static int32_t VBlankCallbackStubGetVersion_(MessageParcel& vBlankCallbackData, MessageParcel& vBlankCallbackReply, MessageOption& vBlankCallbackOption, sptr<OHOS::HDI::Display::Composer::V1_0::IVBlankCallback> impl);

private:
    int32_t VBlankCallbackStubOnVBlank(MessageParcel& vBlankCallbackData, MessageParcel& vBlankCallbackReply, MessageOption& vBlankCallbackOption);

    int32_t VBlankCallbackStubGetVersion(MessageParcel& vBlankCallbackData, MessageParcel& vBlankCallbackReply, MessageOption& vBlankCallbackOption);


    static inline ObjectDelegator<OHOS::HDI::Display::Composer::V1_0::VBlankCallbackStub, OHOS::HDI::Display::Composer::V1_0::IVBlankCallback> objDelegator_;
    sptr<OHOS::HDI::Display::Composer::V1_0::IVBlankCallback> impl_;
};
} // V1_0
} // Composer
} // Display
} // HDI
} // OHOS

#endif // OHOS_HDI_DISPLAY_COMPOSER_V1_0_VBLANKCALLBACKSTUB_H