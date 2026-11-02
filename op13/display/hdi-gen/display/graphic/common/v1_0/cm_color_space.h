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

#ifndef OHOS_HDI_DISPLAY_GRAPHIC_COMMON_V1_0_CMCOLORSPACE_H
#define OHOS_HDI_DISPLAY_GRAPHIC_COMMON_V1_0_CMCOLORSPACE_H

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

enum CM_ColorPrimaries : int32_t {
    COLORPRIMARIES_BT709 = 1,
    COLORPRIMARIES_SRGB = 1,
    COLORPRIMARIES_BT601_P,
    COLORPRIMARIES_BT601_N,
    COLORPRIMARIES_BT2020 = 4,
    COLORPRIMARIES_BT2100 = 4,
    COLORPRIMARIES_P3_DCI,
    COLORPRIMARIES_P3_D65,
    COLORPRIMARIES_MONO,
    COLORPRIMARIES_ADOBERGB = 23,
};

enum CM_TransFunc : int32_t {
    TRANSFUNC_BT709 = 1,
    TRANSFUNC_SRGB = 2,
    TRANSFUNC_LINEAR = 3,
    TRANSFUNC_PQ,
    TRANSFUNC_HLG,
    TRANSFUNC_ADOBERGB,
    TRANSFUNC_GAMMA2_2,
    TRANSFUNC_GAMMA2_4,
};

enum CM_Range : int32_t {
    RANGE_UNKNOW = 0,
    RANGE_FULL,
    RANGE_LIMITED,
    RANGE_EXTEND,
};

enum CM_Matrix : int32_t {
    MATRIX_ADOBERGB = 0,
    MATRIX_BT709 = 1,
    MATRIX_BT601_P,
    MATRIX_BT601_N = 3,
    MATRIX_P3 = 3,
    MATRIX_BT2020 = 4,
    MATRIX_BT2100 = 4,
    MATRIX_BT2100_ICTCP,
};

struct CM_ColorSpaceInfo {
    OHOS::HDI::Display::Graphic::Common::V1_0::CM_ColorPrimaries primaries;
    OHOS::HDI::Display::Graphic::Common::V1_0::CM_TransFunc transfunc;
    OHOS::HDI::Display::Graphic::Common::V1_0::CM_Matrix matrix;
    OHOS::HDI::Display::Graphic::Common::V1_0::CM_Range range;
} __attribute__ ((aligned(8)));

enum CM_ColorSpaceType : int32_t {
    CM_COLORSPACE_NONE,
    CM_BT601_EBU_FULL = 2 | (1 << 8) | (2 << 16) | (1 << 21),
    CM_BT601_SMPTE_C_FULL = 3 | (1 << 8) | (3 << 16) | (1 << 21),
    CM_BT709_FULL = 1 | (1 << 8) | (1 << 16) | (1 << 21),
    CM_BT2020_HLG_FULL = 4 | (5 << 8) | (4 << 16) | (1 << 21),
    CM_BT2020_PQ_FULL = 4 | (4 << 8) | (4 << 16) | (1 << 21),
    CM_BT601_EBU_LIMIT = 2 | (1 << 8) | (2 << 16) | (2 << 21),
    CM_BT601_SMPTE_C_LIMIT = 3 | (1 << 8) | (3 << 16) | (2 << 21),
    CM_BT709_LIMIT = 1 | (1 << 8) | (1 << 16) | (2 << 21),
    CM_BT2020_HLG_LIMIT = 4 | (5 << 8) | (4 << 16) | (2 << 21),
    CM_BT2020_PQ_LIMIT = 4 | (4 << 8) | (4 << 16) | (2 << 21),
    CM_SRGB_FULL = 1 | (2 << 8) | (3 << 16) | (1 << 21),
    CM_P3_FULL = 6 | (2 << 8) | (3 << 16) | (1 << 21),
    CM_P3_HLG_FULL = 6 | (5 << 8) | (3 << 16) | (1 << 21),
    CM_P3_PQ_FULL = 6 | (4 << 8) | (3 << 16) | (1 << 21),
    CM_ADOBERGB_FULL = 23 | (6 << 8) | (0 << 16) | (1 << 21),
    CM_SRGB_LIMIT = 1 | (2 << 8) | (3 << 16) | (2 << 21),
    CM_P3_LIMIT = 6 | (2 << 8) | (3 << 16) | (2 << 21),
    CM_P3_HLG_LIMIT = 6 | (5 << 8) | (3 << 16) | (2 << 21),
    CM_P3_PQ_LIMIT = 6 | (4 << 8) | (3 << 16) | (2 << 21),
    CM_ADOBERGB_LIMIT = 23 | (6 << 8) | (0 << 16) | (2 << 21),
    CM_LINEAR_SRGB = 1 | (3 << 8),
    CM_LINEAR_BT709 = 1 | (3 << 8),
    CM_LINEAR_P3 = 6 | (3 << 8),
    CM_LINEAR_BT2020 = 4 | (3 << 8),
    CM_DISPLAY_SRGB = 1 | (2 << 8) | (3 << 16) | (1 << 21),
    CM_DISPLAY_P3_SRGB = 6 | (2 << 8) | (3 << 16) | (1 << 21),
    CM_DISPLAY_P3_HLG = 6 | (5 << 8) | (3 << 16) | (1 << 21),
    CM_DISPLAY_P3_PQ = 6 | (4 << 8) | (3 << 16) | (1 << 21),
    CM_DISPLAY_BT2020_SRGB = 4 | (2 << 8) | (4 << 16) | (1 << 21),
    CM_DISPLAY_BT2020_HLG = 4 | (5 << 8) | (4 << 16) | (1 << 21),
    CM_DISPLAY_BT2020_PQ = 4 | (4 << 8) | (4 << 16) | (1 << 21),
};

enum CM_ColorSpaceMask : int32_t {
    CM_PRIMARIES_MASK = 255,
    CM_TRANSFUNC_MASK = 255 << 8,
    CM_MATRIX_MASK = 31 << 16,
    CM_RANGE_MASK = 7 << 21,
};

enum CM_HDR_Metadata_Type : int32_t {
    CM_METADATA_NONE,
    CM_VIDEO_HLG,
    CM_VIDEO_HDR10,
    CM_VIDEO_HDR_VIVID,
    CM_IMAGE_HDR_VIVID_DUAL,
    CM_IMAGE_HDR_VIVID_SINGLE,
    CM_IMAGE_HDR_ISO_DUAL,
    CM_IMAGE_HDR_ISO_SINGLE,
};

bool CM_ColorSpaceInfoBlockMarshalling(OHOS::MessageParcel &data, const OHOS::HDI::Display::Graphic::Common::V1_0::CM_ColorSpaceInfo& dataBlock);

bool CM_ColorSpaceInfoBlockUnmarshalling(OHOS::MessageParcel &data, OHOS::HDI::Display::Graphic::Common::V1_0::CM_ColorSpaceInfo& dataBlock);


} // V1_0
} // Common
} // Graphic
} // Display
} // HDI
} // OHOS

#endif // OHOS_HDI_DISPLAY_GRAPHIC_COMMON_V1_0_CMCOLORSPACE_H
