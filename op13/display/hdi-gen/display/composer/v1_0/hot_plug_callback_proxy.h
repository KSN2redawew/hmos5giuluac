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

#ifndef OHOS_HDI_DISPLAY_COMPOSER_V1_0_HOTPLUGCALLBACKPROXY_H
#define OHOS_HDI_DISPLAY_COMPOSER_V1_0_HOTPLUGCALLBACKPROXY_H

#include "v1_0/ihot_plug_callback.h"
#include <iproxy_broker.h>

namespace OHOS {
namespace HDI {
namespace Display {
namespace Composer {
namespace V1_0 {

class HotPlugCallbackProxy : public IProxyBroker<OHOS::HDI::Display::Composer::V1_0::IHotPlugCallback> {
public:
    explicit HotPlugCallbackProxy(const sptr<IRemoteObject>& remote) : IProxyBroker<OHOS::HDI::Display::Composer::V1_0::IHotPlugCallback>(remote) {
    }
    virtual ~HotPlugCallbackProxy() = default;

    inline bool IsProxy() override
    {
        return true;
    }

    int32_t OnHotPlug(uint32_t outputId, bool connected) override;

    int32_t GetVersion(uint32_t& majorVer, uint32_t& minorVer) override;

    static int32_t OnHotPlug_(uint32_t outputId, bool connected, const sptr<IRemoteObject> remote);

    static int32_t GetVersion_(uint32_t& majorVer, uint32_t& minorVer, const sptr<IRemoteObject> remote);

private:
    static inline BrokerDelegator<OHOS::HDI::Display::Composer::V1_0::HotPlugCallbackProxy> delegator_;
};

} // V1_0
} // Composer
} // Display
} // HDI
} // OHOS

#endif // OHOS_HDI_DISPLAY_COMPOSER_V1_0_HOTPLUGCALLBACKPROXY_H