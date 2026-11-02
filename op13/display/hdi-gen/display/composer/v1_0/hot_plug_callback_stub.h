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

#ifndef OHOS_HDI_DISPLAY_COMPOSER_V1_0_HOTPLUGCALLBACKSTUB_H
#define OHOS_HDI_DISPLAY_COMPOSER_V1_0_HOTPLUGCALLBACKSTUB_H

#include <ipc_object_stub.h>
#include <message_option.h>
#include <message_parcel.h>
#include <object_collector.h>
#include <refbase.h>
#include "v1_0/ihot_plug_callback.h"

namespace OHOS {
namespace HDI {
namespace Display {
namespace Composer {
namespace V1_0 {

using namespace OHOS;
class HotPlugCallbackStub : public IPCObjectStub {
public:
    explicit HotPlugCallbackStub(const sptr<IHotPlugCallback> &impl);
    virtual ~HotPlugCallbackStub();

    int32_t OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;

    static int32_t HotPlugCallbackStubOnHotPlug_(MessageParcel& hotPlugCallbackData, MessageParcel& hotPlugCallbackReply, MessageOption& hotPlugCallbackOption, sptr<OHOS::HDI::Display::Composer::V1_0::IHotPlugCallback> impl);

    static int32_t HotPlugCallbackStubGetVersion_(MessageParcel& hotPlugCallbackData, MessageParcel& hotPlugCallbackReply, MessageOption& hotPlugCallbackOption, sptr<OHOS::HDI::Display::Composer::V1_0::IHotPlugCallback> impl);

private:
    int32_t HotPlugCallbackStubOnHotPlug(MessageParcel& hotPlugCallbackData, MessageParcel& hotPlugCallbackReply, MessageOption& hotPlugCallbackOption);

    int32_t HotPlugCallbackStubGetVersion(MessageParcel& hotPlugCallbackData, MessageParcel& hotPlugCallbackReply, MessageOption& hotPlugCallbackOption);


    static inline ObjectDelegator<OHOS::HDI::Display::Composer::V1_0::HotPlugCallbackStub, OHOS::HDI::Display::Composer::V1_0::IHotPlugCallback> objDelegator_;
    sptr<OHOS::HDI::Display::Composer::V1_0::IHotPlugCallback> impl_;
};
} // V1_0
} // Composer
} // Display
} // HDI
} // OHOS

#endif // OHOS_HDI_DISPLAY_COMPOSER_V1_0_HOTPLUGCALLBACKSTUB_H