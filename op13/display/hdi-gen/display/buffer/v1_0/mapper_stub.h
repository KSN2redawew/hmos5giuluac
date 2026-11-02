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

#ifndef OHOS_HDI_DISPLAY_BUFFER_V1_0_MAPPERSTUB_H
#define OHOS_HDI_DISPLAY_BUFFER_V1_0_MAPPERSTUB_H

#include <ipc_object_stub.h>
#include <message_option.h>
#include <message_parcel.h>
#include <object_collector.h>
#include <refbase.h>
#include "v1_0/imapper.h"

namespace OHOS {
namespace HDI {
namespace Display {
namespace Buffer {
namespace V1_0 {

using namespace OHOS;
class MapperStub : public IPCObjectStub {
public:
    explicit MapperStub(const sptr<IMapper> &impl);
    virtual ~MapperStub();

    int32_t OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;

    static int32_t MapperStubFreeMem_(MessageParcel& mapperData, MessageParcel& mapperReply, MessageOption& mapperOption, sptr<OHOS::HDI::Display::Buffer::V1_0::IMapper> impl);

    static int32_t MapperStubMmap_(MessageParcel& mapperData, MessageParcel& mapperReply, MessageOption& mapperOption, sptr<OHOS::HDI::Display::Buffer::V1_0::IMapper> impl);

    static int32_t MapperStubUnmap_(MessageParcel& mapperData, MessageParcel& mapperReply, MessageOption& mapperOption, sptr<OHOS::HDI::Display::Buffer::V1_0::IMapper> impl);

    static int32_t MapperStubFlushCache_(MessageParcel& mapperData, MessageParcel& mapperReply, MessageOption& mapperOption, sptr<OHOS::HDI::Display::Buffer::V1_0::IMapper> impl);

    static int32_t MapperStubInvalidateCache_(MessageParcel& mapperData, MessageParcel& mapperReply, MessageOption& mapperOption, sptr<OHOS::HDI::Display::Buffer::V1_0::IMapper> impl);

    static int32_t MapperStubGetVersion_(MessageParcel& mapperData, MessageParcel& mapperReply, MessageOption& mapperOption, sptr<OHOS::HDI::Display::Buffer::V1_0::IMapper> impl);

private:
    int32_t MapperStubFreeMem(MessageParcel& mapperData, MessageParcel& mapperReply, MessageOption& mapperOption);

    int32_t MapperStubMmap(MessageParcel& mapperData, MessageParcel& mapperReply, MessageOption& mapperOption);

    int32_t MapperStubUnmap(MessageParcel& mapperData, MessageParcel& mapperReply, MessageOption& mapperOption);

    int32_t MapperStubFlushCache(MessageParcel& mapperData, MessageParcel& mapperReply, MessageOption& mapperOption);

    int32_t MapperStubInvalidateCache(MessageParcel& mapperData, MessageParcel& mapperReply, MessageOption& mapperOption);

    int32_t MapperStubGetVersion(MessageParcel& mapperData, MessageParcel& mapperReply, MessageOption& mapperOption);


    static inline ObjectDelegator<OHOS::HDI::Display::Buffer::V1_0::MapperStub, OHOS::HDI::Display::Buffer::V1_0::IMapper> objDelegator_;
    sptr<OHOS::HDI::Display::Buffer::V1_0::IMapper> impl_;
};
} // V1_0
} // Buffer
} // Display
} // HDI
} // OHOS

#endif // OHOS_HDI_DISPLAY_BUFFER_V1_0_MAPPERSTUB_H