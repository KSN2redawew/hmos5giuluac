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

#ifndef OHOS_HDI_DISPLAY_COMPOSER_V1_1_IDISPLAYCOMPOSER_H
#define OHOS_HDI_DISPLAY_COMPOSER_V1_1_IDISPLAYCOMPOSER_H

#include <stdint.h>
#include <string>
#include <vector>
#include <hdf_base.h>
#include <hdi_base.h>
#include "display/composer/v1_0/display_composer_type.h"
#include "display/composer/v1_0/idisplay_composer.h"
#include "display/composer/v1_0/irefresh_callback.h"
#include "display/composer/v1_1/display_composer_type.h"
#include "display/composer/v1_1/imode_callback.h"
#include "display/composer/v1_1/iseamless_change_callback.h"

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
namespace HDI {
namespace Display {
namespace Composer {
namespace V1_1 {
using namespace OHOS;
using namespace OHOS::HDI;
using namespace OHOS::HDI::Display::Composer::V1_0;

enum {
    CMD_DISPLAY_COMPOSER_REG_SEAMLESS_CHANGE_CALLBACK = 25,
    CMD_DISPLAY_COMPOSER_GET_DISPLAY_SUPPORTED_MODES_EXT = 26,
    CMD_DISPLAY_COMPOSER_SET_DISPLAY_MODE_ASYNC = 27,
    CMD_DISPLAY_COMPOSER_GET_DISPLAY_V_BLANK_PERIOD = 28,
    CMD_DISPLAY_COMPOSER_SET_LAYER_PER_FRAME_PARAMETER = 29,
    CMD_DISPLAY_COMPOSER_GET_SUPPORTED_LAYER_PER_FRAME_PARAMETER_KEY = 30,
    CMD_DISPLAY_COMPOSER_SET_DISPLAY_OVERLAY_RESOLUTION = 31,
    CMD_DISPLAY_COMPOSER_REG_REFRESH_CALLBACK = 32,
    CMD_DISPLAY_COMPOSER_GET_DISPLAY_SUPPORTED_COLOR_GAMUTS = 33,
    CMD_DISPLAY_COMPOSER_GET_H_D_R_CAPABILITY_INFOS = 34,
};

class IDisplayComposer : public OHOS::HDI::Display::Composer::V1_0::IDisplayComposer {
public:
    DECLARE_HDI_DESCRIPTOR(u"ohos.hdi.display.composer.v1_1.IDisplayComposer");

    virtual ~IDisplayComposer() = default;

    static sptr<OHOS::HDI::Display::Composer::V1_1::IDisplayComposer> Get(bool isStub = false);
    static sptr<OHOS::HDI::Display::Composer::V1_1::IDisplayComposer> Get(const std::string &serviceName, bool isStub = false);

    static sptr<OHOS::HDI::Display::Composer::V1_1::IDisplayComposer> CastFrom(const sptr<OHOS::HDI::Display::Composer::V1_0::IDisplayComposer> &parent);

    virtual int32_t RegSeamlessChangeCallback(const sptr<OHOS::HDI::Display::Composer::V1_1::ISeamlessChangeCallback>& cb) = 0;

    virtual int32_t GetDisplaySupportedModesExt(uint32_t devId,
         std::vector<OHOS::HDI::Display::Composer::V1_1::DisplayModeInfoExt>& modes) = 0;

    virtual int32_t SetDisplayModeAsync(uint32_t devId, uint32_t modeId,
         const sptr<OHOS::HDI::Display::Composer::V1_1::IModeCallback>& cb) = 0;

    virtual int32_t GetDisplayVBlankPeriod(uint32_t devId, uint64_t& period) = 0;

    virtual int32_t SetLayerPerFrameParameter(uint32_t devId, uint32_t layerId, const std::string& key,
         const std::vector<int8_t>& value) = 0;

    virtual int32_t GetSupportedLayerPerFrameParameterKey(std::vector<std::string>& keys) = 0;

    virtual int32_t SetDisplayOverlayResolution(uint32_t devId, uint32_t width, uint32_t height) = 0;

    virtual int32_t RegRefreshCallback(const sptr<OHOS::HDI::Display::Composer::V1_0::IRefreshCallback>& cb) = 0;

    virtual int32_t GetDisplaySupportedColorGamuts(uint32_t devId,
         std::vector<OHOS::HDI::Display::Composer::V1_0::ColorGamut>& gamuts) = 0;

    virtual int32_t GetHDRCapabilityInfos(uint32_t devId, OHOS::HDI::Display::Composer::V1_0::HDRCapability& info) = 0;

    int32_t GetVersion(uint32_t& majorVer, uint32_t& minorVer) override
    {
        majorVer = 1;
        minorVer = 1;
        return HDF_SUCCESS;
    }

    const std::u16string GetDesc() override
    {
        return metaDescriptor_;
    }
};
} // V1_1
} // Composer
} // Display
} // HDI
} // OHOS

#endif // OHOS_HDI_DISPLAY_COMPOSER_V1_1_IDISPLAYCOMPOSER_H