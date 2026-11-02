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

#ifndef OHOS_HDI_DISPLAY_BUFFER_V1_2_MAPPERPROXY_H
#define OHOS_HDI_DISPLAY_BUFFER_V1_2_MAPPERPROXY_H

#include "v1_0/mapper_proxy.h"
#include "v1_2/imapper.h"
#include <unistd.h>
#include <iproxy_broker.h>

namespace OHOS {
namespace HDI {
namespace Display {
namespace Buffer {
namespace V1_2 {

class MapperProxy : public IProxyBroker<OHOS::HDI::Display::Buffer::V1_2::IMapper> {
public:
    class IServiceManagerDeathRecipient : public IRemoteObject::DeathRecipient {
    public:
        IServiceManagerDeathRecipient(wptr<OHOS::HDI::Display::Buffer::V1_2::MapperProxy> proxy) : proxy_(proxy) {}
        ~IServiceManagerDeathRecipient() override = default;
        void OnRemoteDied(const wptr<IRemoteObject> &remote) override
        {
            int32_t result = HDF_FAILURE;
            const int sleepInterval = 500000;
            const int waitTimes = 10;
            int currentTime = waitTimes;
            do {
                usleep(sleepInterval);
                auto proxy = proxy_.promote();
                if (proxy != nullptr) {
                    result = OHOS::HDI::Display::Buffer::V1_2::MapperProxy::Reconnect(proxy);
                }
                --currentTime;
            } while (result != HDF_SUCCESS && currentTime >0);
        }
    private:
        wptr<OHOS::HDI::Display::Buffer::V1_2::MapperProxy> proxy_;
    };

    explicit MapperProxy(const sptr<IRemoteObject>& remote) : IProxyBroker<OHOS::HDI::Display::Buffer::V1_2::IMapper>(remote) {
        reconnectRemote_ = nullptr;
        servMgr_ = nullptr;
        deathRecipient_ = nullptr;
        isReconnected_ = false;
    }
    virtual ~MapperProxy() {
        if (servMgr_ != nullptr && deathRecipient_ != nullptr) {
            servMgr_->RemoveDeathRecipient(deathRecipient_);
        }
    }

    inline bool IsProxy() override
    {
        return true;
    }

    int32_t GetImageLayout(const sptr<NativeBuffer>& handle,
         OHOS::HDI::Display::Buffer::V1_2::ImageLayout& layout) override;

    int32_t FreeMem(const sptr<NativeBuffer>& handle) override;

    int32_t Mmap(const sptr<NativeBuffer>& handle) override;

    int32_t Unmap(const sptr<NativeBuffer>& handle) override;

    int32_t FlushCache(const sptr<NativeBuffer>& handle) override;

    int32_t InvalidateCache(const sptr<NativeBuffer>& handle) override;

    int32_t GetVersion(uint32_t& majorVer, uint32_t& minorVer) override;

    static int32_t GetImageLayout_(const sptr<NativeBuffer>& handle,
         OHOS::HDI::Display::Buffer::V1_2::ImageLayout& layout, const sptr<IRemoteObject> remote);

    static int32_t Reconnect(sptr<OHOS::HDI::Display::Buffer::V1_2::MapperProxy> proxy);

    sptr<IRemoteObject> GetCurrentRemote() {
        return isReconnected_ ? reconnectRemote_ : Remote();
    }

    bool isReconnected_;
    std::string serviceName_;
    sptr<IRemoteObject> servMgr_;
    sptr<OHOS::HDI::Display::Buffer::V1_2::MapperProxy::IServiceManagerDeathRecipient> deathRecipient_;
    sptr<IRemoteObject> reconnectRemote_;
private:
    static inline BrokerDelegator<OHOS::HDI::Display::Buffer::V1_2::MapperProxy> delegator_;
};

} // V1_2
} // Buffer
} // Display
} // HDI
} // OHOS

#endif // OHOS_HDI_DISPLAY_BUFFER_V1_2_MAPPERPROXY_H
