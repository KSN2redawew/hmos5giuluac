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

#ifndef OHOS_HDI_DISPLAY_COMPOSER_V1_0_IDISPLAYCOMPOSER_H
#define OHOS_HDI_DISPLAY_COMPOSER_V1_0_IDISPLAYCOMPOSER_H

#include <stdint.h>
#include <vector>
#include <base/hdi_smq.h>
#include <base/native_buffer.h>
#include <hdf_base.h>
#include <hdi_base.h>
#include "display/composer/v1_0/display_composer_type.h"
#include "display/composer/v1_0/ihot_plug_callback.h"
#include "display/composer/v1_0/irefresh_callback.h"
#include "display/composer/v1_0/iv_blank_callback.h"
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
namespace V1_0 {
using namespace OHOS;
using namespace OHOS::HDI;
using namespace OHOS::HDI::Base;
using namespace OHOS::HDI::Display;

enum {
    CMD_DISPLAY_COMPOSER_GET_VERSION = 0,
    CMD_DISPLAY_COMPOSER_REG_HOT_PLUG_CALLBACK = 1,
    CMD_DISPLAY_COMPOSER_SET_CLIENT_BUFFER_CACHE_COUNT = 2,
    CMD_DISPLAY_COMPOSER_REG_DISPLAY_V_BLANK_CALLBACK = 3,
    CMD_DISPLAY_COMPOSER_GET_DISPLAY_CAPABILITY = 4,
    CMD_DISPLAY_COMPOSER_GET_DISPLAY_SUPPORTED_MODES = 5,
    CMD_DISPLAY_COMPOSER_GET_DISPLAY_MODE = 6,
    CMD_DISPLAY_COMPOSER_SET_DISPLAY_MODE = 7,
    CMD_DISPLAY_COMPOSER_GET_DISPLAY_POWER_STATUS = 8,
    CMD_DISPLAY_COMPOSER_SET_DISPLAY_POWER_STATUS = 9,
    CMD_DISPLAY_COMPOSER_GET_DISPLAY_BACKLIGHT = 10,
    CMD_DISPLAY_COMPOSER_SET_DISPLAY_BACKLIGHT = 11,
    CMD_DISPLAY_COMPOSER_SET_DISPLAY_VSYNC_ENABLED = 12,
    CMD_DISPLAY_COMPOSER_CREATE_LAYER = 13,
    CMD_DISPLAY_COMPOSER_DESTROY_LAYER = 14,
    CMD_DISPLAY_COMPOSER_SET_DISPLAY_CLIENT_CROP = 15,
    CMD_DISPLAY_COMPOSER_GET_DISPLAY_RELEASE_FENCE = 16,
    CMD_DISPLAY_COMPOSER_CREATE_VIRTUAL_DISPLAY = 17,
    CMD_DISPLAY_COMPOSER_DESTROY_VIRTUAL_DISPLAY = 18,
    CMD_DISPLAY_COMPOSER_SET_VIRTUAL_DISPLAY_BUFFER = 19,
    CMD_DISPLAY_COMPOSER_SET_DISPLAY_PROPERTY = 20,
    CMD_DISPLAY_COMPOSER_GET_DISPLAY_PROPERTY = 21,
    CMD_DISPLAY_COMPOSER_INIT_CMD_REQUEST = 22,
    CMD_DISPLAY_COMPOSER_CMD_REQUEST = 23,
    CMD_DISPLAY_COMPOSER_GET_CMD_REPLY = 24,
};

class IDisplayComposer : public HdiBase {
public:
    DECLARE_HDI_DESCRIPTOR(u"ohos.hdi.display.composer.v1_0.IDisplayComposer");

    virtual ~IDisplayComposer() = default;

    static sptr<OHOS::HDI::Display::Composer::V1_0::IDisplayComposer> Get(bool isStub = false);
    static sptr<OHOS::HDI::Display::Composer::V1_0::IDisplayComposer> Get(const std::string &serviceName, bool isStub = false);

    virtual int32_t RegHotPlugCallback(const sptr<OHOS::HDI::Display::Composer::V1_0::IHotPlugCallback>& cb) = 0;

    virtual int32_t SetClientBufferCacheCount(uint32_t devId, uint32_t count) = 0;

    virtual int32_t RegDisplayVBlankCallback(uint32_t devId,
         const sptr<OHOS::HDI::Display::Composer::V1_0::IVBlankCallback>& cb) = 0;

    virtual int32_t GetDisplayCapability(uint32_t devId,
         OHOS::HDI::Display::Composer::V1_0::DisplayCapability& info) = 0;

    virtual int32_t GetDisplaySupportedModes(uint32_t devId,
         std::vector<OHOS::HDI::Display::Composer::V1_0::DisplayModeInfo>& modes) = 0;

    virtual int32_t GetDisplayMode(uint32_t devId, uint32_t& modeId) = 0;

    virtual int32_t SetDisplayMode(uint32_t devId, uint32_t modeId) = 0;

    virtual int32_t GetDisplayPowerStatus(uint32_t devId,
         OHOS::HDI::Display::Composer::V1_0::DispPowerStatus& status) = 0;

    virtual int32_t SetDisplayPowerStatus(uint32_t devId,
         OHOS::HDI::Display::Composer::V1_0::DispPowerStatus status) = 0;

    virtual int32_t GetDisplayBacklight(uint32_t devId, uint32_t& level) = 0;

    virtual int32_t SetDisplayBacklight(uint32_t devId, uint32_t level) = 0;

    virtual int32_t SetDisplayVsyncEnabled(uint32_t devId, bool enabled) = 0;

    virtual int32_t CreateLayer(uint32_t devId, const OHOS::HDI::Display::Composer::V1_0::LayerInfo& layerInfo,
         uint32_t cacheCount, uint32_t& layerId) = 0;

    virtual int32_t DestroyLayer(uint32_t devId, uint32_t layerId) = 0;

    virtual int32_t SetDisplayClientCrop(uint32_t devId, const OHOS::HDI::Display::Composer::V1_0::IRect& rect) = 0;

    virtual int32_t GetDisplayReleaseFence(uint32_t devId, std::vector<uint32_t>& layers,
         std::vector<sptr<OHOS::HDI::Display::HdifdParcelable>>& fences) = 0;

    virtual int32_t CreateVirtualDisplay(uint32_t width, uint32_t height, int32_t& format, uint32_t& devId) = 0;

    virtual int32_t DestroyVirtualDisplay(uint32_t devId) = 0;

    virtual int32_t SetVirtualDisplayBuffer(uint32_t devId, const sptr<NativeBuffer>& buffer,
         const sptr<OHOS::HDI::Display::HdifdParcelable>& fence) = 0;

    virtual int32_t SetDisplayProperty(uint32_t devId, uint32_t id, uint64_t value) = 0;

    virtual int32_t GetDisplayProperty(uint32_t devId, uint32_t id, uint64_t& value) = 0;

    virtual int32_t InitCmdRequest(const std::shared_ptr<SharedMemQueue<int32_t>>& request) = 0;

    virtual int32_t CmdRequest(uint32_t inEleCnt,
         const std::vector<OHOS::HDI::Display::Composer::V1_0::HdifdInfo>& inFds, uint32_t& outEleCnt, std::vector<OHOS::HDI::Display::Composer::V1_0::HdifdInfo>& outFds) = 0;

    virtual int32_t GetCmdReply(std::shared_ptr<SharedMemQueue<int32_t>>& reply) = 0;

    virtual int32_t GetVersion(uint32_t& majorVer, uint32_t& minorVer)
    {
        majorVer = 1;
        minorVer = 0;
        return HDF_SUCCESS;
    }

    virtual bool IsProxy()
    {
        return false;
    }

    virtual const std::u16string GetDesc()
    {
        return metaDescriptor_;
    }
};
} // V1_0
} // Composer
} // Display
} // HDI
} // OHOS

#endif // OHOS_HDI_DISPLAY_COMPOSER_V1_0_IDISPLAYCOMPOSER_H