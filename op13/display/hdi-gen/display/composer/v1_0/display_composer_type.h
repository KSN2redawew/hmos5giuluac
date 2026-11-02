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

#ifndef OHOS_HDI_DISPLAY_COMPOSER_V1_0_DISPLAYCOMPOSERTYPE_H
#define OHOS_HDI_DISPLAY_COMPOSER_V1_0_DISPLAYCOMPOSERTYPE_H

#include <cstdbool>
#include <cstdint>
#include <string>
#include <vector>
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
namespace V1_0 {

using namespace OHOS;
using namespace OHOS::HDI::Display;

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
};

enum DispErrCode : int32_t {
    DISPLAY_SUCCESS = 0,
    DISPLAY_FAILURE = -1,
    DISPLAY_FD_ERR = -2,
    DISPLAY_PARAM_ERR = -3,
    DISPLAY_NULL_PTR = -4,
    DISPLAY_NOT_SUPPORT = -5,
    DISPLAY_NOMEM = -6,
    DISPLAY_SYS_BUSY = -7,
    DISPLAY_NOT_PERM = -8,
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
};

enum TransformType : int32_t {
    ROTATE_NONE = 0,
    ROTATE_90,
    ROTATE_180,
    ROTATE_270,
    MIRROR_H,
    MIRROR_V,
    MIRROR_H_ROTATE_90,
    MIRROR_V_ROTATE_90,
    ROTATE_BUTT,
};

enum DispPowerStatus : int32_t {
    POWER_STATUS_ON = 0,
    POWER_STATUS_STANDBY = 1,
    POWER_STATUS_SUSPEND = 2,
    POWER_STATUS_OFF = 3,
    POWER_STATUS_BUTT,
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
};

enum LayerType : int32_t {
    LAYER_TYPE_GRAPHIC,
    LAYER_TYPE_OVERLAY,
    LAYER_TYPE_SDIEBAND,
    LAYER_TYPE_CURSOR,
    LAYER_TYPE_BUTT,
};

enum BlendType : int32_t {
    BLEND_NONE = 0,
    BLEND_CLEAR,
    BLEND_SRC,
    BLEND_SRCOVER,
    BLEND_DSTOVER,
    BLEND_SRCIN,
    BLEND_DSTIN,
    BLEND_SRCOUT,
    BLEND_DSTOUT,
    BLEND_SRCATOP,
    BLEND_DSTATOP,
    BLEND_ADD,
    BLEND_XOR,
    BLEND_DST,
    BLEND_AKS,
    BLEND_AKD,
    BLEND_BUTT,
};

enum RopType : int32_t {
    ROP_BLACK = 0,
    ROP_NOTMERGEPEN,
    ROP_MASKNOTPEN,
    ROP_NOTCOPYPEN,
    ROP_MASKPENNOT,
    ROP_NOT,
    ROP_XORPEN,
    ROP_NOTMASKPEN,
    ROP_MASKPEN,
    ROP_NOTXORPEN,
    ROP_NOP,
    ROP_MERGENOTPEN,
    ROP_COPYPE,
    ROP_MERGEPENNOT,
    ROP_MERGEPEN,
    ROP_WHITE,
    ROP_BUTT,
};

enum ColorKey : int32_t {
    CKEY_NONE = 0,
    CKEY_SRC,
    CKEY_DST,
    CKEY_BUTT,
};

enum MirrorType : int32_t {
    MIRROR_NONE = 0,
    MIRROR_LR,
    MIRROR_TB,
    MIRROR_BUTT,
};

enum Connection : int32_t {
    CON_INVALID = 0,
    CONNECTED,
    DISCONNECTED,
};

enum InterfaceType : int32_t {
    DISP_INTF_HDMI = 0,
    DISP_INTF_LCD,
    DISP_INTF_BT1120,
    DISP_INTF_BT656,
    DISP_INTF_YPBPR,
    DISP_INTF_RGB,
    DISP_INTF_CVBS,
    DISP_INTF_SVIDEO,
    DISP_INTF_VGA,
    DISP_INTF_MIPI,
    DISP_INTF_PANEL,
    DISP_INTF_BUTT,
};

struct PropertyObject {
    std::string name;
    uint32_t propId;
    uint64_t value;
};

struct DisplayCapability {
    std::string name;
    OHOS::HDI::Display::Composer::V1_0::InterfaceType type;
    uint32_t phyWidth;
    uint32_t phyHeight;
    uint32_t supportLayers;
    uint32_t virtualDispCount;
    bool supportWriteBack;
    uint32_t propertyCount;
    std::vector<OHOS::HDI::Display::Composer::V1_0::PropertyObject> props;
};

struct DisplayModeInfo {
    int32_t width;
    int32_t height;
    uint32_t freshRate;
    int32_t id;
} __attribute__ ((aligned(8)));

struct LayerInfo {
    int32_t width;
    int32_t height;
    OHOS::HDI::Display::Composer::V1_0::LayerType type;
    int32_t bpp;
    OHOS::HDI::Display::Composer::V1_0::PixelFormat pixFormat;
} __attribute__ ((aligned(8)));

struct LayerAlpha {
    bool enGlobalAlpha;
    bool enPixelAlpha;
    uint8_t alpha0;
    uint8_t alpha1;
    uint8_t gAlpha;
} __attribute__ ((aligned(8)));

struct IRect {
    int32_t x;
    int32_t y;
    int32_t w;
    int32_t h;
} __attribute__ ((aligned(8)));

struct ISurface {
    uint64_t phyAddr;
    int32_t height;
    int32_t width;
    int32_t stride;
    OHOS::HDI::Display::Composer::V1_0::PixelFormat enColorFmt;
    bool bYCbCrClut;
    bool bAlphaMax255;
    bool bAlphaExt1555;
    uint8_t alpha0;
    uint8_t alpha1;
    uint64_t cbcrPhyAddr;
    int32_t cbcrStride;
    uint64_t clutPhyAddr;
} __attribute__ ((aligned(8)));

struct ILine {
    int32_t x0;
    int32_t y0;
    int32_t x1;
    int32_t y1;
    uint32_t color;
} __attribute__ ((aligned(8)));

struct ICircle {
    int32_t x;
    int32_t y;
    int32_t r;
    uint32_t color;
} __attribute__ ((aligned(8)));

struct Rectangle {
    OHOS::HDI::Display::Composer::V1_0::IRect rect;
    uint32_t color;
} __attribute__ ((aligned(8)));

struct GfxOpt {
    bool enGlobalAlpha;
    uint32_t globalAlpha;
    bool enPixelAlpha;
    OHOS::HDI::Display::Composer::V1_0::BlendType blendType;
    OHOS::HDI::Display::Composer::V1_0::ColorKey colorKeyFrom;
    bool enableRop;
    OHOS::HDI::Display::Composer::V1_0::RopType colorRopType;
    OHOS::HDI::Display::Composer::V1_0::RopType alphaRopType;
    bool enableScale;
    OHOS::HDI::Display::Composer::V1_0::TransformType rotateType;
    OHOS::HDI::Display::Composer::V1_0::MirrorType mirrorType;
} __attribute__ ((aligned(8)));

enum ColorGamut : int32_t {
    COLOR_GAMUT_INVALID = -1,
    COLOR_GAMUT_NATIVE = 0,
    COLOR_GAMUT_STANDARD_BT601 = 1,
    COLOR_GAMUT_STANDARD_BT709 = 2,
    COLOR_GAMUT_DCI_P3 = 3,
    COLOR_GAMUT_SRGB = 4,
    COLOR_GAMUT_ADOBE_RGB = 5,
    COLOR_GAMUT_DISPLAY_P3 = 6,
    COLOR_GAMUT_BT2020 = 7,
    COLOR_GAMUT_BT2100_PQ = 8,
    COLOR_GAMUT_BT2100_HLG = 9,
    COLOR_GAMUT_DISPLAY_BT2020 = 10,
};

enum GamutMap : int32_t {
    GAMUT_MAP_CONSTANT = 0,
    GAMUT_MAP_EXPANSION = 1,
    GAMUT_MAP_HDR_CONSTANT = 2,
    GAMUT_MAP_HDR_EXPANSION = 3,
};

enum ColorDataSpace : int32_t {
    COLOR_DATA_SPACE_UNKNOWN = 0,
    GAMUT_BT601 = 0x00000001,
    GAMUT_BT709 = 0x00000002,
    GAMUT_DCI_P3 = 0x00000003,
    GAMUT_SRGB = 0x00000004,
    GAMUT_ADOBE_RGB = 0x00000005,
    GAMUT_DISPLAY_P3 = 0x00000006,
    GAMUT_BT2020 = 0x00000007,
    GAMUT_BT2100_PQ = 0x00000008,
    GAMUT_BT2100_HLG = 0x00000009,
    GAMUT_DISPLAY_BT2020 = 0x0000000a,
    TRANSFORM_FUNC_UNSPECIFIED = 0x00000100,
    TRANSFORM_FUNC_LINEAR = 0x00000200,
    TRANSFORM_FUNC_SRGB = 0x00000300,
    TRANSFORM_FUNC_SMPTE_170M = 0x00000400,
    TRANSFORM_FUNC_GM2_2 = 0x00000500,
    TRANSFORM_FUNC_GM2_6 = 0x00000600,
    TRANSFORM_FUNC_GM2_8 = 0x00000700,
    TRANSFORM_FUNC_ST2084 = 0x00000800,
    TRANSFORM_FUNC_HLG = 0x00000900,
    PRECISION_UNSPECIFIED = 0x00010000,
    PRECISION_FULL = 0x00020000,
    PRESION_LIMITED = 0x00030000,
    PRESION_EXTENDED = 0x00040000,
    BT601_SMPTE170M_FULL = 1 | 1024 | 131072,
    BT601_SMPTE170M_LIMITED = 1 | 1024 | 196608,
    BT709_LINEAR_FULL = 2 | 512 | 131072,
    BT709_LINEAR_EXTENDED = 2 | 512 | 262144,
    BT709_SRGB_FULL = 2 | 768 | 131072,
    BT709_SRGB_EXTENDED = 2 | 768 | 262144,
    BT709_SMPTE170M_LIMITED = 2 | 1024 | 196608,
    DCI_P3_LINEAR_FULL = 3 | 512 | 131072,
    DCI_P3_GAMMA26_FULL = 3 | 1536 | 131072,
    DISPLAY_P3_LINEAR_FULL = 6 | 512 | 131072,
    DCI_P3_SRGB_FULL = 3 | 768 | 131072,
    ADOBE_RGB_GAMMA22_FULL = 5 | 1280 | 131072,
    BT2020_LINEAR_FULL = 7 | 512 | 131072,
    BT2020_SRGB_FULL = 7 | 768 | 131072,
    BT2020_SMPTE170M_FULL = 7 | 1024 | 131072,
    BT2020_ST2084_FULL = 7 | 2048 | 131072,
    BT2020_HLG_FULL = 7 | 2304 | 131072,
    BT2020_ST2084_LIMITED = 7 | 2048 | 196608,
};

enum HDRFormat : int32_t {
    NOT_SUPPORT_HDR = 0,
    DOLBY_VISION = 1,
    HDR10 = 2,
    HLG = 3,
    HDR10_PLUS = 4,
    HDR_VIVID = 5,
};

struct HDRCapability {
    uint32_t formatCount;
    std::vector<OHOS::HDI::Display::Composer::V1_0::HDRFormat> formats;
    float maxLum;
    float maxAverageLum;
    float minLum;
};

enum HDRMetadataKey : int32_t {
    MATAKEY_RED_PRIMARY_X = 0,
    MATAKEY_RED_PRIMARY_Y = 1,
    MATAKEY_GREEN_PRIMARY_X = 2,
    MATAKEY_GREEN_PRIMARY_Y = 3,
    MATAKEY_BLUE_PRIMARY_X = 4,
    MATAKEY_BLUE_PRIMARY_Y = 5,
    MATAKEY_WHITE_PRIMARY_X = 6,
    MATAKEY_WHITE_PRIMARY_Y = 7,
    MATAKEY_MAX_LUMINANCE = 8,
    MATAKEY_MIN_LUMINANCE = 9,
    MATAKEY_MAX_CONTENT_LIGHT_LEVEL = 10,
    MATAKEY_MAX_FRAME_AVERAGE_LIGHT_LEVEL = 11,
    MATAKEY_HDR10_PLUS = 12,
    MATAKEY_HDR_VIVID = 13,
};

struct HDRMetaData {
    OHOS::HDI::Display::Composer::V1_0::HDRMetadataKey key;
    float value;
} __attribute__ ((aligned(8)));

enum PresentTimestampType : int32_t {
    HARDWARE_DISPLAY_PTS_UNSUPPORTED = 0,
    HARDWARE_DISPLAY_PTS_DELAY = 1 << 0,
    HARDWARE_DISPLAY_PTS_TIMESTAMP = 1 << 1,
};

enum MaskInfo : int32_t {
    LAYER_NORAML = 0,
    LAYER_HBM_SYNC = 1,
};

struct PresentTimestamp {
    OHOS::HDI::Display::Composer::V1_0::PresentTimestampType type;
    int64_t time;
} __attribute__ ((aligned(8)));

struct ExtDataHandle {
    int32_t fd;
    uint32_t reserveInts;
    std::vector<int32_t> reserve;
};

struct YUVDescInfo {
    uint64_t baseAddr;
    uint32_t yOffset;
    uint32_t uOffset;
    uint32_t vOffset;
    uint32_t yStride;
    uint32_t uvStride;
    uint32_t uvStep;
} __attribute__ ((aligned(8)));

struct HdifdInfo {
    int32_t id;
    sptr<OHOS::HDI::Display::HdifdParcelable> hdiFd;
};

struct LayerColor {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} __attribute__ ((aligned(8)));

bool PropertyObjectBlockMarshalling(OHOS::MessageParcel &data, const OHOS::HDI::Display::Composer::V1_0::PropertyObject& dataBlock);

bool PropertyObjectBlockUnmarshalling(OHOS::MessageParcel &data, OHOS::HDI::Display::Composer::V1_0::PropertyObject& dataBlock);

bool DisplayCapabilityBlockMarshalling(OHOS::MessageParcel &data, const OHOS::HDI::Display::Composer::V1_0::DisplayCapability& dataBlock);

bool DisplayCapabilityBlockUnmarshalling(OHOS::MessageParcel &data, OHOS::HDI::Display::Composer::V1_0::DisplayCapability& dataBlock);

bool DisplayModeInfoBlockMarshalling(OHOS::MessageParcel &data, const OHOS::HDI::Display::Composer::V1_0::DisplayModeInfo& dataBlock);

bool DisplayModeInfoBlockUnmarshalling(OHOS::MessageParcel &data, OHOS::HDI::Display::Composer::V1_0::DisplayModeInfo& dataBlock);

bool LayerInfoBlockMarshalling(OHOS::MessageParcel &data, const OHOS::HDI::Display::Composer::V1_0::LayerInfo& dataBlock);

bool LayerInfoBlockUnmarshalling(OHOS::MessageParcel &data, OHOS::HDI::Display::Composer::V1_0::LayerInfo& dataBlock);

bool LayerAlphaBlockMarshalling(OHOS::MessageParcel &data, const OHOS::HDI::Display::Composer::V1_0::LayerAlpha& dataBlock);

bool LayerAlphaBlockUnmarshalling(OHOS::MessageParcel &data, OHOS::HDI::Display::Composer::V1_0::LayerAlpha& dataBlock);

bool IRectBlockMarshalling(OHOS::MessageParcel &data, const OHOS::HDI::Display::Composer::V1_0::IRect& dataBlock);

bool IRectBlockUnmarshalling(OHOS::MessageParcel &data, OHOS::HDI::Display::Composer::V1_0::IRect& dataBlock);

bool ISurfaceBlockMarshalling(OHOS::MessageParcel &data, const OHOS::HDI::Display::Composer::V1_0::ISurface& dataBlock);

bool ISurfaceBlockUnmarshalling(OHOS::MessageParcel &data, OHOS::HDI::Display::Composer::V1_0::ISurface& dataBlock);

bool ILineBlockMarshalling(OHOS::MessageParcel &data, const OHOS::HDI::Display::Composer::V1_0::ILine& dataBlock);

bool ILineBlockUnmarshalling(OHOS::MessageParcel &data, OHOS::HDI::Display::Composer::V1_0::ILine& dataBlock);

bool ICircleBlockMarshalling(OHOS::MessageParcel &data, const OHOS::HDI::Display::Composer::V1_0::ICircle& dataBlock);

bool ICircleBlockUnmarshalling(OHOS::MessageParcel &data, OHOS::HDI::Display::Composer::V1_0::ICircle& dataBlock);

bool RectangleBlockMarshalling(OHOS::MessageParcel &data, const OHOS::HDI::Display::Composer::V1_0::Rectangle& dataBlock);

bool RectangleBlockUnmarshalling(OHOS::MessageParcel &data, OHOS::HDI::Display::Composer::V1_0::Rectangle& dataBlock);

bool GfxOptBlockMarshalling(OHOS::MessageParcel &data, const OHOS::HDI::Display::Composer::V1_0::GfxOpt& dataBlock);

bool GfxOptBlockUnmarshalling(OHOS::MessageParcel &data, OHOS::HDI::Display::Composer::V1_0::GfxOpt& dataBlock);

bool HDRCapabilityBlockMarshalling(OHOS::MessageParcel &data, const OHOS::HDI::Display::Composer::V1_0::HDRCapability& dataBlock);

bool HDRCapabilityBlockUnmarshalling(OHOS::MessageParcel &data, OHOS::HDI::Display::Composer::V1_0::HDRCapability& dataBlock);

bool HDRMetaDataBlockMarshalling(OHOS::MessageParcel &data, const OHOS::HDI::Display::Composer::V1_0::HDRMetaData& dataBlock);

bool HDRMetaDataBlockUnmarshalling(OHOS::MessageParcel &data, OHOS::HDI::Display::Composer::V1_0::HDRMetaData& dataBlock);

bool PresentTimestampBlockMarshalling(OHOS::MessageParcel &data, const OHOS::HDI::Display::Composer::V1_0::PresentTimestamp& dataBlock);

bool PresentTimestampBlockUnmarshalling(OHOS::MessageParcel &data, OHOS::HDI::Display::Composer::V1_0::PresentTimestamp& dataBlock);

bool ExtDataHandleBlockMarshalling(OHOS::MessageParcel &data, const OHOS::HDI::Display::Composer::V1_0::ExtDataHandle& dataBlock);

bool ExtDataHandleBlockUnmarshalling(OHOS::MessageParcel &data, OHOS::HDI::Display::Composer::V1_0::ExtDataHandle& dataBlock);

bool YUVDescInfoBlockMarshalling(OHOS::MessageParcel &data, const OHOS::HDI::Display::Composer::V1_0::YUVDescInfo& dataBlock);

bool YUVDescInfoBlockUnmarshalling(OHOS::MessageParcel &data, OHOS::HDI::Display::Composer::V1_0::YUVDescInfo& dataBlock);

bool HdifdInfoBlockMarshalling(OHOS::MessageParcel &data, const OHOS::HDI::Display::Composer::V1_0::HdifdInfo& dataBlock);

bool HdifdInfoBlockUnmarshalling(OHOS::MessageParcel &data, OHOS::HDI::Display::Composer::V1_0::HdifdInfo& dataBlock);

bool LayerColorBlockMarshalling(OHOS::MessageParcel &data, const OHOS::HDI::Display::Composer::V1_0::LayerColor& dataBlock);

bool LayerColorBlockUnmarshalling(OHOS::MessageParcel &data, OHOS::HDI::Display::Composer::V1_0::LayerColor& dataBlock);

} // V1_0
} // Composer
} // Display
} // HDI
} // OHOS

#endif // OHOS_HDI_DISPLAY_COMPOSER_V1_0_DISPLAYCOMPOSERTYPE_H