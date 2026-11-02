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

#ifndef OHOS_HDI_DISPLAY_COMPOSER_V1_2_IDISPLAYCOMPOSER_H
#define OHOS_HDI_DISPLAY_COMPOSER_V1_2_IDISPLAYCOMPOSER_H

#include <stdint.h>
#include <hdf_base.h>
#include <hdi_base.h>
#include "display/composer/v1_0/display_composer_type.h"
#include "display/composer/v1_1/idisplay_composer.h"
#include "display/composer/v1_2/display_composer_type.h"
#include "display/composer/v1_2/iv_blank_idle_callback.h"
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
namespace HDI {
namespace Display {
namespace Composer {
namespace V1_2 {
using namespace OHOS;
using namespace OHOS::HDI;
using namespace OHOS::HDI::Display::Composer::V1_1;
using namespace OHOS::HDI::Display::Composer::V1_0;
using namespace OHOS::HDI::Display;

enum {
    CMD_DISPLAY_COMPOSER_REG_DISPLAY_V_BLANK_IDLE_CALLBACK = 35,
    CMD_DISPLAY_COMPOSER_CLEAR_CLIENT_BUFFER = 36,
    CMD_DISPLAY_COMPOSER_CLEAR_LAYER_BUFFER = 37,
    CMD_DISPLAY_COMPOSER_SET_HARDWARE_CURSOR_POSITION = 38,
    CMD_DISPLAY_COMPOSER_ENABLE_HARDWARE_CURSOR_STATS = 39,
    CMD_DISPLAY_COMPOSER_GET_HARDWARE_CURSOR_STATS = 40,
    CMD_DISPLAY_COMPOSER_SET_DISPLAY_ACTIVE_REGION = 41,
};

class IDisplayComposer : public OHOS::HDI::Display::Composer::V1_1::IDisplayComposer {
public:
    DECLARE_HDI_DESCRIPTOR(u"ohos.hdi.display.composer.v1_2.IDisplayComposer");

    virtual ~IDisplayComposer() = default;

    static sptr<OHOS::HDI::Display::Composer::V1_2::IDisplayComposer> Get(bool isStub = false);
    static sptr<OHOS::HDI::Display::Composer::V1_2::IDisplayComposer> Get(const std::string &serviceName, bool isStub = false);

    static sptr<OHOS::HDI::Display::Composer::V1_2::IDisplayComposer> CastFrom(const sptr<OHOS::HDI::Display::Composer::V1_1::IDisplayComposer> &parent);
    static sptr<OHOS::HDI::Display::Composer::V1_2::IDisplayComposer> CastFrom(const sptr<OHOS::HDI::Display::Composer::V1_0::IDisplayComposer> &parent);

    virtual int32_t RegDisplayVBlankIdleCallback(const sptr<OHOS::HDI::Display::Composer::V1_2::IVBlankIdleCallback>& cb) = 0;

    virtual int32_t ClearClientBuffer(uint32_t devId) = 0;

    virtual int32_t ClearLayerBuffer(uint32_t devId, uint32_t layerId) = 0;

    virtual int32_t SetHardwareCursorPosition(uint32_t devId, int32_t x, int32_t y) = 0;

    virtual int32_t EnableHardwareCursorStats(uint32_t devId, bool enable) = 0;

    virtual int32_t GetHardwareCursorStats(uint32_t devId, uint32_t& frameCount, uint32_t& vsyncCount) = 0;

    virtual int32_t SetDisplayActiveRegion(uint32_t devId, const OHOS::HDI::Display::Composer::V1_0::IRect& rect) = 0;

    int32_t GetVersion(uint32_t& majorVer, uint32_t& minorVer) override
    {
        majorVer = 1;
        minorVer = 2;
        return HDF_SUCCESS;
    }

    const std::u16string GetDesc() override
    {
        return metaDescriptor_;
    }
};
} // V1_2
} // Composer
} // Display
} // HDI
} // OHOS

#endif // OHOS_HDI_DISPLAY_COMPOSER_V1_2_IDISPLAYCOMPOSER_H