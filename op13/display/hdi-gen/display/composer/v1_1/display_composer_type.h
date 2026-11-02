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

#ifndef OHOS_HDI_DISPLAY_COMPOSER_V1_1_DISPLAYCOMPOSERTYPE_H
#define OHOS_HDI_DISPLAY_COMPOSER_V1_1_DISPLAYCOMPOSERTYPE_H

#include <cstdbool>
#include <cstdint>
#include "display/composer/v1_0/display_composer_type.h"
#include "hdifd_parcelable.h"

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
namespace Composer {
namespace V1_1 {

using namespace OHOS;
using namespace OHOS::HDI::Display;
using namespace OHOS::HDI::Display::Composer::V1_0;

enum PixelFormat : int32_t {
    PIXEL_FMT_CLUT8 = 0,
    PIXEL_FMT_CLUT1,
    PIXEL_FMT_CLUT4,
    PIXEL_FMT_RGB_565,
    PIXEL_FMT_RGBA_5658,
    PIXEL_FMT_RGBX_4444,
    PIXEL_FMT_RGBA_4444,
    PIXEL_FMT_RGB_444,
    PIXEL_FMT_RGBX_5551,
    PIXEL_FMT_RGBA_5551,
    PIXEL_FMT_RGB_555,
    PIXEL_FMT_RGBX_8888,
    PIXEL_FMT_RGBA_8888,
    PIXEL_FMT_RGB_888,
    PIXEL_FMT_BGR_565,
    PIXEL_FMT_BGRX_4444,
    PIXEL_FMT_BGRA_4444,
    PIXEL_FMT_BGRX_5551,
    PIXEL_FMT_BGRA_5551,
    PIXEL_FMT_BGRX_8888,
    PIXEL_FMT_BGRA_8888,
    PIXEL_FMT_YUV_422_I,
    PIXEL_FMT_YCBCR_422_SP,
    PIXEL_FMT_YCRCB_422_SP,
    PIXEL_FMT_YCBCR_420_SP,
    PIXEL_FMT_YCRCB_420_SP,
    PIXEL_FMT_YCBCR_422_P,
    PIXEL_FMT_YCRCB_422_P,
    PIXEL_FMT_YCBCR_420_P,
    PIXEL_FMT_YCRCB_420_P,
    PIXEL_FMT_YUYV_422_PKG,
    PIXEL_FMT_UYVY_422_PKG,
    PIXEL_FMT_YVYU_422_PKG,
    PIXEL_FMT_VYUY_422_PKG,
    PIXEL_FMT_RGBA_1010102,
    PIXEL_FMT_VENDER_MASK = 0X7FFF0000,
    PIXEL_FMT_BUTT = 0X7FFFFFFF,
    PIXEL_FMT_YCBCR_P010 = 35,
    PIXEL_FMT_YCRCB_P010,
    PIXEL_FMT_RAW10,
    PIXEL_FMT_BLOB,
};

enum DispPowerStatus : int32_t {
    POWER_STATUS_ON = 0,
    POWER_STATUS_STANDBY = 1,
    POWER_STATUS_SUSPEND = 2,
    POWER_STATUS_OFF = 3,
    POWER_STATUS_BUTT,
    POWER_STATUS_OFF_FAKE = 4,
    POWER_STATUS_BUTT_V1_1,
};

enum CompositionType : int32_t {
    COMPOSITION_CLIENT,
    COMPOSITION_DEVICE,
    COMPOSITION_CURSOR,
    COMPOSITION_VIDEO,
    COMPOSITION_DEVICE_CLEAR,
    COMPOSITION_CLIENT_CLEAR,
    COMPOSITION_TUNNEL,
    COMPOSITION_BUTT,
    COMPOSITION_SOLID_COLOR = 7,
    COMPOSITION_BUTT_V1_1,
};

struct DisplayModeInfoExt {
    OHOS::HDI::Display::Composer::V1_0::DisplayModeInfo v1_0;
    uint32_t groupId;
} __attribute__ ((aligned(8)));

bool DisplayModeInfoExtBlockMarshalling(OHOS::MessageParcel &data, const OHOS::HDI::Display::Composer::V1_1::DisplayModeInfoExt& dataBlock);

bool DisplayModeInfoExtBlockUnmarshalling(OHOS::MessageParcel &data, OHOS::HDI::Display::Composer::V1_1::DisplayModeInfoExt& dataBlock);

} // V1_1
} // Composer
} // Display
} // HDI
} // OHOS

#endif // OHOS_HDI_DISPLAY_COMPOSER_V1_1_DISPLAYCOMPOSERTYPE_H