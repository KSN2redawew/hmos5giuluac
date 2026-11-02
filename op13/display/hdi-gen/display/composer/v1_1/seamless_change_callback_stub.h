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

#ifndef OHOS_HDI_DISPLAY_COMPOSER_V1_1_SEAMLESSCHANGECALLBACKSTUB_H
#define OHOS_HDI_DISPLAY_COMPOSER_V1_1_SEAMLESSCHANGECALLBACKSTUB_H

#include <ipc_object_stub.h>
#include <message_option.h>
#include <message_parcel.h>
#include <object_collector.h>
#include <refbase.h>
#include "v1_1/iseamless_change_callback.h"

namespace OHOS {
namespace HDI {
namespace Display {
namespace Composer {
namespace V1_1 {

using namespace OHOS;
class SeamlessChangeCallbackStub : public IPCObjectStub {
public:
    explicit SeamlessChangeCallbackStub(const sptr<ISeamlessChangeCallback> &impl);
    virtual ~SeamlessChangeCallbackStub();

    int32_t OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;

    static int32_t SeamlessChangeCallbackStubOnSeamlessChange_(MessageParcel& seamlessChangeCallbackData, MessageParcel& seamlessChangeCallbackReply, MessageOption& seamlessChangeCallbackOption, sptr<OHOS::HDI::Display::Composer::V1_1::ISeamlessChangeCallback> impl);

    static int32_t SeamlessChangeCallbackStubGetVersion_(MessageParcel& seamlessChangeCallbackData, MessageParcel& seamlessChangeCallbackReply, MessageOption& seamlessChangeCallbackOption, sptr<OHOS::HDI::Display::Composer::V1_1::ISeamlessChangeCallback> impl);

private:
    int32_t SeamlessChangeCallbackStubOnSeamlessChange(MessageParcel& seamlessChangeCallbackData, MessageParcel& seamlessChangeCallbackReply, MessageOption& seamlessChangeCallbackOption);

    int32_t SeamlessChangeCallbackStubGetVersion(MessageParcel& seamlessChangeCallbackData, MessageParcel& seamlessChangeCallbackReply, MessageOption& seamlessChangeCallbackOption);


    static inline ObjectDelegator<OHOS::HDI::Display::Composer::V1_1::SeamlessChangeCallbackStub, OHOS::HDI::Display::Composer::V1_1::ISeamlessChangeCallback> objDelegator_;
    sptr<OHOS::HDI::Display::Composer::V1_1::ISeamlessChangeCallback> impl_;
};
} // V1_1
} // Composer
} // Display
} // HDI
} // OHOS

#endif // OHOS_HDI_DISPLAY_COMPOSER_V1_1_SEAMLESSCHANGECALLBACKSTUB_H