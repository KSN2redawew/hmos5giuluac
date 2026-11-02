/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_HDI_DISPLAY_COMPOSER_V1_2_VBLANKIDLECALLBACKSTUB_H
#define OHOS_HDI_DISPLAY_COMPOSER_V1_2_VBLANKIDLECALLBACKSTUB_H

#include <ipc_object_stub.h>
#include <message_option.h>
#include <message_parcel.h>
#include <object_collector.h>
#include <refbase.h>
#include "v1_2/iv_blank_idle_callback.h"

namespace OHOS {
namespace HDI {
namespace Display {
namespace Composer {
namespace V1_2 {

using namespace OHOS;
class VBlankIdleCallbackStub : public IPCObjectStub {
public:
    explicit VBlankIdleCallbackStub(const sptr<IVBlankIdleCallback> &impl);
    virtual ~VBlankIdleCallbackStub();

    int32_t OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;

    static int32_t VBlankIdleCallbackStubOnVBlankIdleCallback_(MessageParcel& vBlankIdleCallbackData, MessageParcel& vBlankIdleCallbackReply, MessageOption& vBlankIdleCallbackOption, sptr<OHOS::HDI::Display::Composer::V1_2::IVBlankIdleCallback> impl);

    static int32_t VBlankIdleCallbackStubGetVersion_(MessageParcel& vBlankIdleCallbackData, MessageParcel& vBlankIdleCallbackReply, MessageOption& vBlankIdleCallbackOption, sptr<OHOS::HDI::Display::Composer::V1_2::IVBlankIdleCallback> impl);

private:
    int32_t VBlankIdleCallbackStubOnVBlankIdleCallback(MessageParcel& vBlankIdleCallbackData, MessageParcel& vBlankIdleCallbackReply, MessageOption& vBlankIdleCallbackOption);

    int32_t VBlankIdleCallbackStubGetVersion(MessageParcel& vBlankIdleCallbackData, MessageParcel& vBlankIdleCallbackReply, MessageOption& vBlankIdleCallbackOption);


    static inline ObjectDelegator<OHOS::HDI::Display::Composer::V1_2::VBlankIdleCallbackStub, OHOS::HDI::Display::Composer::V1_2::IVBlankIdleCallback> objDelegator_;
    sptr<OHOS::HDI::Display::Composer::V1_2::IVBlankIdleCallback> impl_;
};
} // V1_2
} // Composer
} // Display
} // HDI
} // OHOS

#endif // OHOS_HDI_DISPLAY_COMPOSER_V1_2_VBLANKIDLECALLBACKSTUB_H