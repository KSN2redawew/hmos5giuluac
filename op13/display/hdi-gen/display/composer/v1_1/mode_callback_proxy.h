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

#ifndef OHOS_HDI_DISPLAY_COMPOSER_V1_1_MODECALLBACKPROXY_H
#define OHOS_HDI_DISPLAY_COMPOSER_V1_1_MODECALLBACKPROXY_H

#include "v1_1/imode_callback.h"
#include <iproxy_broker.h>

namespace OHOS {
namespace HDI {
namespace Display {
namespace Composer {
namespace V1_1 {

class ModeCallbackProxy : public IProxyBroker<OHOS::HDI::Display::Composer::V1_1::IModeCallback> {
public:
    explicit ModeCallbackProxy(const sptr<IRemoteObject>& remote) : IProxyBroker<OHOS::HDI::Display::Composer::V1_1::IModeCallback>(remote) {
    }
    virtual ~ModeCallbackProxy() = default;

    inline bool IsProxy() override
    {
        return true;
    }

    int32_t OnMode(uint32_t modeId, uint64_t vBlankPeriod) override;

    int32_t GetVersion(uint32_t& majorVer, uint32_t& minorVer) override;

    static int32_t OnMode_(uint32_t modeId, uint64_t vBlankPeriod, const sptr<IRemoteObject> remote);

    static int32_t GetVersion_(uint32_t& majorVer, uint32_t& minorVer, const sptr<IRemoteObject> remote);

private:
    static inline BrokerDelegator<OHOS::HDI::Display::Composer::V1_1::ModeCallbackProxy> delegator_;
};

} // V1_1
} // Composer
} // Display
} // HDI
} // OHOS

#endif // OHOS_HDI_DISPLAY_COMPOSER_V1_1_MODECALLBACKPROXY_H