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

#ifndef OHOS_HDI_DISPLAY_COMPOSER_V1_2_DISPLAYCOMPOSERTYPE_H
#define OHOS_HDI_DISPLAY_COMPOSER_V1_2_DISPLAYCOMPOSERTYPE_H

#include <cstdbool>
#include <cstdint>
#include "display/composer/v1_0/display_composer_type.h"
#include "display/composer/v1_1/display_composer_type.h"
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
namespace V1_2 {

using namespace OHOS;
using namespace OHOS::HDI::Display;
using namespace OHOS::HDI::Display::Composer::V1_0;
using namespace OHOS::HDI::Display::Composer::V1_1;

enum DispPowerStatus : int32_t {
    POWER_STATUS_ON = 0,
    POWER_STATUS_STANDBY = 1,
    POWER_STATUS_SUSPEND = 2,
    POWER_STATUS_OFF = 3,
    POWER_STATUS_BUTT,
    POWER_STATUS_OFF_FAKE = 4,
    POWER_STATUS_BUTT_V1_1,
    POWER_STATUS_PRE_ON = 5,
    POWER_STATUS_PRE_OFF = 6,
    POWER_STATUS_BUTT_V1_2,
};

enum BufferUsage : uint64_t {
    HBM_USE_CPU_READ = (1ULL << 0),
    HBM_USE_CPU_WRITE = (1ULL << 1),
    HBM_USE_MEM_MMZ = (1ULL << 2),
    HBM_USE_MEM_DMA = (1ULL << 3),
    HBM_USE_MEM_SHARE = (1ULL << 4),
    HBM_USE_MEM_MMZ_CACHE = (1ULL << 5),
    HBM_USE_MEM_FB = (1ULL << 6),
    HBM_USE_ASSIGN_SIZE = (1ULL << 7),
    HBM_USE_HW_RENDER = (1ULL << 8),
    HBM_USE_HW_TEXTURE = (1ULL << 9),
    HBM_USE_HW_COMPOSER = (1ULL << 10),
    HBM_USE_PROTECTED = (1ULL << 11),
    HBM_USE_CAMERA_READ = (1ULL << 12),
    HBM_USE_CAMERA_WRITE = (1ULL << 13),
    HBM_USE_VIDEO_ENCODER = (1ULL << 14),
    HBM_USE_VIDEO_DECODER = (1ULL << 15),
    HBM_USE_CPU_READ_OFTEN = (1ULL << 16),
    HBM_USE_VENDOR_PRI0 = (1ULL << 44),
    HBM_USE_VENDOR_PRI1 = (1ULL << 45),
    HBM_USE_VENDOR_PRI2 = (1ULL << 46),
    HBM_USE_VENDOR_PRI3 = (1ULL << 47),
    HBM_USE_VENDOR_PRI4 = (1ULL << 48),
    HBM_USE_VENDOR_PRI5 = (1ULL << 49),
    HBM_USE_VENDOR_PRI6 = (1ULL << 50),
    HBM_USE_VENDOR_PRI7 = (1ULL << 51),
    HBM_USE_VENDOR_PRI8 = (1ULL << 52),
    HBM_USE_VENDOR_PRI9 = (1ULL << 53),
    HBM_USE_VENDOR_PRI10 = (1ULL << 54),
    HBM_USE_VENDOR_PRI11 = (1ULL << 55),
    HBM_USE_VENDOR_PRI12 = (1ULL << 56),
    HBM_USE_VENDOR_PRI13 = (1ULL << 57),
    HBM_USE_VENDOR_PRI14 = (1ULL << 58),
    HBM_USE_VENDOR_PRI15 = (1ULL << 59),
    HBM_USE_VENDOR_PRI16 = (1ULL << 60),
    HBM_USE_VENDOR_PRI17 = (1ULL << 61),
    HBM_USE_VENDOR_PRI18 = (1ULL << 62),
    HBM_USE_VENDOR_PRI19 = (1ULL << 63),
    HBM_USE_CPU_HW_BOTH = (1ULL << 17),
    HBM_USAGE_GPU_PERFORMANCE = (1ULL << 18),
    HBM_USE_RGB_TO_YUV_CONVERSION = (1ULL << 19),
    HBM_USE_DRM_REDRAW = (1ULL << 24),
};

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
    PIXEL_FMT_RGBA16_FLOAT = 39,
};

enum DispCmd : int32_t {
    REQUEST_CMD_PREPARE_DISPLAY_LAYERS = 64,
    REQUEST_CMD_SET_DISPLAY_CLIENT_BUFFER = 65,
    REQUEST_CMD_SET_DISPLAY_CLIENT_DAMAGE = 66,
    REQUEST_CMD_COMMIT = 67,
    REQUEST_CMD_SET_LAYER_ALPHA = 68,
    REQUEST_CMD_SET_LAYER_REGION = 69,
    REQUEST_CMD_SET_LAYER_CROP = 70,
    REQUEST_CMD_SET_LAYER_ZORDER = 71,
    REQUEST_CMD_SET_LAYER_PREMULTI = 72,
    REQUEST_CMD_SET_LAYER_TRANSFORM_MODE = 73,
    REQUEST_CMD_SET_LAYER_DIRTY_REGION = 74,
    REQUEST_CMD_SET_LAYER_VISIBLE_REGION = 75,
    REQUEST_CMD_SET_LAYER_BUFFER = 76,
    REQUEST_CMD_SET_LAYER_COMPOSITION_TYPE = 77,
    REQUEST_CMD_SET_LAYER_BLEND_TYPE = 78,
    REQUEST_CMD_SET_LAYER_VISIBLE = 79,
    REQUEST_CMD_SET_LAYER_MASK_INFO = 80,
    REQUEST_CMD_SET_LAYER_COLOR = 81,
    REQUEST_CMD_BUTT,
    REPLY_CMD_SET_ERROR = 512,
    REPLY_CMD_PREPARE_DISPLAY_LAYERS = 513,
    REPLY_CMD_COMMIT = 514,
    REPLY_CMD_BUTT,
    CONTROL_CMD_REQUEST_BEGIN = 1024,
    CONTROL_CMD_REPLY_BEGIN = 1025,
    CONTROL_CMD_REQUEST_END = 1026,
    CONTROL_CMD_REPLY_END = 1027,
    CONTROL_CMD_BUTT,
    REQUEST_CMD_COMMIT_AND_GET_RELEASE_FENCE = 82,
    REQUEST_CMD_SET_DISPLAY_CONSTRAINT = 83,
    REQUEST_CMD_BUTT_V1_2,
    REPLY_CMD_COMMIT_AND_GET_RELEASE_FENCE = 515,
    REPLY_CMD_BUTT_V1_2,
};

enum DisplayPropertyID : int32_t {
    DISPLAY_PROPERTY_ID_SKIP_VALIDATE = 1,
    DISPLAY_CAPBILITY_HARDWARE_CURSOR = 2,
    DISPLAY_PROPERTY_ID_ADAPTIVE_SYNC = 3,
};


} // V1_2
} // Composer
} // Display
} // HDI
} // OHOS

#endif // OHOS_HDI_DISPLAY_COMPOSER_V1_2_DISPLAYCOMPOSERTYPE_H