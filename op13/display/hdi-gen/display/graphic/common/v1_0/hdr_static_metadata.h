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

#ifndef OHOS_HDI_DISPLAY_GRAPHIC_COMMON_V1_0_HDRSTATICMETADATA_H
#define OHOS_HDI_DISPLAY_GRAPHIC_COMMON_V1_0_HDRSTATICMETADATA_H

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
namespace Graphic {
namespace Common {
namespace V1_0 {

using namespace OHOS;

struct ColorXY {
    float x;
    float y;
} __attribute__ ((aligned(8)));

struct Smpte2086_Metadata {
    OHOS::HDI::Display::Graphic::Common::V1_0::ColorXY displayPrimaryRed;
    OHOS::HDI::Display::Graphic::Common::V1_0::ColorXY displayPrimaryGreen;
    OHOS::HDI::Display::Graphic::Common::V1_0::ColorXY displayPrimaryBlue;
    OHOS::HDI::Display::Graphic::Common::V1_0::ColorXY whitePoint;
    float maxLuminance;
    float minLuminance;
} __attribute__ ((aligned(8)));

struct Cta861_3_Metadata {
    float maxContentLightLevel;
    float maxFrameAverageLightLevel;
} __attribute__ ((aligned(8)));

struct HdrStaticMetadata {
    OHOS::HDI::Display::Graphic::Common::V1_0::Smpte2086_Metadata smpte2086;
    OHOS::HDI::Display::Graphic::Common::V1_0::Cta861_3_Metadata cta861;
} __attribute__ ((aligned(8)));

bool ColorXYBlockMarshalling(OHOS::MessageParcel &data, const OHOS::HDI::Display::Graphic::Common::V1_0::ColorXY& dataBlock);

bool ColorXYBlockUnmarshalling(OHOS::MessageParcel &data, OHOS::HDI::Display::Graphic::Common::V1_0::ColorXY& dataBlock);

bool Smpte2086_MetadataBlockMarshalling(OHOS::MessageParcel &data, const OHOS::HDI::Display::Graphic::Common::V1_0::Smpte2086_Metadata& dataBlock);

bool Smpte2086_MetadataBlockUnmarshalling(OHOS::MessageParcel &data, OHOS::HDI::Display::Graphic::Common::V1_0::Smpte2086_Metadata& dataBlock);

bool Cta861_3_MetadataBlockMarshalling(OHOS::MessageParcel &data, const OHOS::HDI::Display::Graphic::Common::V1_0::Cta861_3_Metadata& dataBlock);

bool Cta861_3_MetadataBlockUnmarshalling(OHOS::MessageParcel &data, OHOS::HDI::Display::Graphic::Common::V1_0::Cta861_3_Metadata& dataBlock);

bool HdrStaticMetadataBlockMarshalling(OHOS::MessageParcel &data, const OHOS::HDI::Display::Graphic::Common::V1_0::HdrStaticMetadata& dataBlock);

bool HdrStaticMetadataBlockUnmarshalling(OHOS::MessageParcel &data, OHOS::HDI::Display::Graphic::Common::V1_0::HdrStaticMetadata& dataBlock);

} // V1_0
} // Common
} // Graphic
} // Display
} // HDI
} // OHOS

#endif // OHOS_HDI_DISPLAY_GRAPHIC_COMMON_V1_0_HDRSTATICMETADATA_H
