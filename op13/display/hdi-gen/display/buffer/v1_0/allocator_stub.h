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

#ifndef OHOS_HDI_DISPLAY_BUFFER_V1_0_ALLOCATORSTUB_H
#define OHOS_HDI_DISPLAY_BUFFER_V1_0_ALLOCATORSTUB_H

#include <ipc_object_stub.h>
#include <message_option.h>
#include <message_parcel.h>
#include <object_collector.h>
#include <refbase.h>
#include "v1_0/iallocator.h"

namespace OHOS {
namespace HDI {
namespace Display {
namespace Buffer {
namespace V1_0 {

using namespace OHOS;
class AllocatorStub : public IPCObjectStub {
public:
    explicit AllocatorStub(const sptr<IAllocator> &impl);
    virtual ~AllocatorStub();

    int32_t OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;

    static int32_t AllocatorStubAllocMem_(MessageParcel& allocatorData, MessageParcel& allocatorReply, MessageOption& allocatorOption, sptr<OHOS::HDI::Display::Buffer::V1_0::IAllocator> impl);

    static int32_t AllocatorStubGetVersion_(MessageParcel& allocatorData, MessageParcel& allocatorReply, MessageOption& allocatorOption, sptr<OHOS::HDI::Display::Buffer::V1_0::IAllocator> impl);

private:
    int32_t AllocatorStubAllocMem(MessageParcel& allocatorData, MessageParcel& allocatorReply, MessageOption& allocatorOption);

    int32_t AllocatorStubGetVersion(MessageParcel& allocatorData, MessageParcel& allocatorReply, MessageOption& allocatorOption);


    static inline ObjectDelegator<OHOS::HDI::Display::Buffer::V1_0::AllocatorStub, OHOS::HDI::Display::Buffer::V1_0::IAllocator> objDelegator_;
    sptr<OHOS::HDI::Display::Buffer::V1_0::IAllocator> impl_;
};
} // V1_0
} // Buffer
} // Display
} // HDI
} // OHOS

#endif // OHOS_HDI_DISPLAY_BUFFER_V1_0_ALLOCATORSTUB_H