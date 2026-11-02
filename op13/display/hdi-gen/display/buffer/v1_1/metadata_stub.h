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

#ifndef OHOS_HDI_DISPLAY_BUFFER_V1_1_METADATASTUB_H
#define OHOS_HDI_DISPLAY_BUFFER_V1_1_METADATASTUB_H

#include <ipc_object_stub.h>
#include <message_option.h>
#include <message_parcel.h>
#include <object_collector.h>
#include <refbase.h>
#include "v1_1/imetadata.h"

namespace OHOS {
namespace HDI {
namespace Display {
namespace Buffer {
namespace V1_1 {

using namespace OHOS;
class MetadataStub : public IPCObjectStub {
public:
    explicit MetadataStub(const sptr<IMetadata> &impl);
    virtual ~MetadataStub();

    int32_t OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;

    static int32_t MetadataStubRegisterBuffer_(MessageParcel& metadataData, MessageParcel& metadataReply, MessageOption& metadataOption, sptr<OHOS::HDI::Display::Buffer::V1_1::IMetadata> impl);

    static int32_t MetadataStubSetMetadata_(MessageParcel& metadataData, MessageParcel& metadataReply, MessageOption& metadataOption, sptr<OHOS::HDI::Display::Buffer::V1_1::IMetadata> impl);

    static int32_t MetadataStubGetMetadata_(MessageParcel& metadataData, MessageParcel& metadataReply, MessageOption& metadataOption, sptr<OHOS::HDI::Display::Buffer::V1_1::IMetadata> impl);

    static int32_t MetadataStubListMetadataKeys_(MessageParcel& metadataData, MessageParcel& metadataReply, MessageOption& metadataOption, sptr<OHOS::HDI::Display::Buffer::V1_1::IMetadata> impl);

    static int32_t MetadataStubEraseMetadataKey_(MessageParcel& metadataData, MessageParcel& metadataReply, MessageOption& metadataOption, sptr<OHOS::HDI::Display::Buffer::V1_1::IMetadata> impl);

    static int32_t MetadataStubGetVersion_(MessageParcel& metadataData, MessageParcel& metadataReply, MessageOption& metadataOption, sptr<OHOS::HDI::Display::Buffer::V1_1::IMetadata> impl);

private:
    int32_t MetadataStubRegisterBuffer(MessageParcel& metadataData, MessageParcel& metadataReply, MessageOption& metadataOption);

    int32_t MetadataStubSetMetadata(MessageParcel& metadataData, MessageParcel& metadataReply, MessageOption& metadataOption);

    int32_t MetadataStubGetMetadata(MessageParcel& metadataData, MessageParcel& metadataReply, MessageOption& metadataOption);

    int32_t MetadataStubListMetadataKeys(MessageParcel& metadataData, MessageParcel& metadataReply, MessageOption& metadataOption);

    int32_t MetadataStubEraseMetadataKey(MessageParcel& metadataData, MessageParcel& metadataReply, MessageOption& metadataOption);

    int32_t MetadataStubGetVersion(MessageParcel& metadataData, MessageParcel& metadataReply, MessageOption& metadataOption);


    static inline ObjectDelegator<OHOS::HDI::Display::Buffer::V1_1::MetadataStub, OHOS::HDI::Display::Buffer::V1_1::IMetadata> objDelegator_;
    sptr<OHOS::HDI::Display::Buffer::V1_1::IMetadata> impl_;
};
} // V1_1
} // Buffer
} // Display
} // HDI
} // OHOS

#endif // OHOS_HDI_DISPLAY_BUFFER_V1_1_METADATASTUB_H