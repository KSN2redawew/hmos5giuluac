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

#ifndef OHOS_HDI_DISPLAY_BUFFER_V1_0_DISPLAYBUFFERTYPE_H
#define OHOS_HDI_DISPLAY_BUFFER_V1_0_DISPLAYBUFFERTYPE_H

#include <cstdbool>
#include <cstdint>

#ifndef HDI_BUFF_MAX_SIZE
#define HDI_BUFF_MAX_SIZE (1024 * 200)
#endif

#ifndef HDI_CHECK_VALUE_RETURN
#define HDI_CHECK_VALUE_RETURN(lv, compare, rv, ret) do { \
    if ((lv) compare (rv)) { \
        return ret; \
    } \
} while (false)
#endif

#ifndef HDI_CHECK_VALUE_RET_GOTO
#define HDI_CHECK_VALUE_RET_GOTO(lv, compare, rv, ret, value, table) do { \
    if ((lv) compare (rv)) { \
        ret = value; \
        goto table; \
    } \
} while (false)
#endif

namespace OHOS {
class MessageParcel;
}

namespace OHOS {
namespace HDI {
namespace Display {
namespace Buffer {
namespace V1_0 {

using namespace OHOS;

struct AllocInfo {
    uint32_t width;
    uint32_t height;
    uint64_t usage;
    uint32_t format;
    uint32_t expectedSize;
} __attribute__ ((aligned(8)));

struct VerifyAllocInfo {
    uint32_t width;
    uint32_t height;
    uint64_t usage;
    uint32_t format;
} __attribute__ ((aligned(8)));

bool AllocInfoBlockMarshalling(OHOS::MessageParcel &data, const OHOS::HDI::Display::Buffer::V1_0::AllocInfo& dataBlock);

bool AllocInfoBlockUnmarshalling(OHOS::MessageParcel &data, OHOS::HDI::Display::Buffer::V1_0::AllocInfo& dataBlock);

bool VerifyAllocInfoBlockMarshalling(OHOS::MessageParcel &data, const OHOS::HDI::Display::Buffer::V1_0::VerifyAllocInfo& dataBlock);

bool VerifyAllocInfoBlockUnmarshalling(OHOS::MessageParcel &data, OHOS::HDI::Display::Buffer::V1_0::VerifyAllocInfo& dataBlock);

} // V1_0
} // Buffer
} // Display
} // HDI
} // OHOS

#endif // OHOS_HDI_DISPLAY_BUFFER_V1_0_DISPLAYBUFFERTYPE_H