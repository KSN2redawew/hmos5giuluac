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

#ifndef OHOS_HDI_DISPLAY_GRAPHIC_COMMON_V1_1_BUFFERHANDLEMETAKEYTYPE_H
#define OHOS_HDI_DISPLAY_GRAPHIC_COMMON_V1_1_BUFFERHANDLEMETAKEYTYPE_H

#include <cstdbool>
#include <cstdint>
#include "display/graphic/common/v1_0/buffer_handle_meta_key_type.h"

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
namespace Graphic {
namespace Common {
namespace V1_1 {

using namespace OHOS;
using namespace OHOS::HDI::Display::Graphic::Common::V1_0;

enum BufferHandleAttrKey : int32_t {
    ATTRKEY_COLORSPACE_INFO = 1,
    ATTRKEY_COLORSPACE_TYPE,
    ATTRKEY_HDR_METADATA_TYPE,
    ATTRKEY_HDR_STATIC_METADATA,
    ATTRKEY_HDR_DYNAMIC_METADATA,
    ATTRKEY_HDR_PROCESSED,
    ATTRKEY_CROP_REGION,
    ATTRKEY_EXPECT_FPS,
    ATTRKEY_DATA_ACCESS,
    ATTRKEY_VENDOR_EXT_START = 2048,
    ATTRKEY_OEM_EXT_START = 4096,
    ATTRKEY_END = 8192,
    ATTRKEY_REQUEST_ACCESS_TYPE = 10,
    ATTRKEY_ACCESS_TYPE = 11,
};

enum HebcAccessType : int32_t {
    HEBC_ACCESS_UNINIT = 0,
    HEBC_ACCESS_CPU_ACCESS,
    HEBC_ACCESS_HW_ONLY,
};


} // V1_1
} // Common
} // Graphic
} // Display
} // HDI
} // OHOS

#endif // OHOS_HDI_DISPLAY_GRAPHIC_COMMON_V1_1_BUFFERHANDLEMETAKEYTYPE_H