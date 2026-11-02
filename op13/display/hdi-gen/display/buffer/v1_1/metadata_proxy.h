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

#ifndef OHOS_HDI_DISPLAY_BUFFER_V1_1_METADATAPROXY_H
#define OHOS_HDI_DISPLAY_BUFFER_V1_1_METADATAPROXY_H

#include "v1_1/imetadata.h"
#include <unistd.h>
#include <iproxy_broker.h>

namespace OHOS {
namespace HDI {
namespace Display {
namespace Buffer {
namespace V1_1 {

class MetadataProxy : public IProxyBroker<OHOS::HDI::Display::Buffer::V1_1::IMetadata> {
public:
    class IServiceManagerDeathRecipient : public IRemoteObject::DeathRecipient {
    public:
        IServiceManagerDeathRecipient(wptr<OHOS::HDI::Display::Buffer::V1_1::MetadataProxy> proxy) : proxy_(proxy) {}
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
                    result = OHOS::HDI::Display::Buffer::V1_1::MetadataProxy::Reconnect(proxy);
                }
                --currentTime;
            } while (result != HDF_SUCCESS && currentTime >0);
        }
    private:
        wptr<OHOS::HDI::Display::Buffer::V1_1::MetadataProxy> proxy_;
    };

    explicit MetadataProxy(const sptr<IRemoteObject>& remote) : IProxyBroker<OHOS::HDI::Display::Buffer::V1_1::IMetadata>(remote) {
        reconnectRemote_ = nullptr;
        servMgr_ = nullptr;
        deathRecipient_ = nullptr;
        isReconnected_ = false;
    }
    virtual ~MetadataProxy() {
        if (servMgr_ != nullptr && deathRecipient_ != nullptr) {
            servMgr_->RemoveDeathRecipient(deathRecipient_);
        }
    }

    inline bool IsProxy() override
    {
        return true;
    }

    int32_t RegisterBuffer(const sptr<NativeBuffer>& handle) override;

    int32_t SetMetadata(const sptr<NativeBuffer>& handle, uint32_t key, const std::vector<uint8_t>& value) override;

    int32_t GetMetadata(const sptr<NativeBuffer>& handle, uint32_t key, std::vector<uint8_t>& value) override;

    int32_t ListMetadataKeys(const sptr<NativeBuffer>& handle, std::vector<uint32_t>& keys) override;

    int32_t EraseMetadataKey(const sptr<NativeBuffer>& handle, uint32_t key) override;

    int32_t GetVersion(uint32_t& majorVer, uint32_t& minorVer) override;

    static int32_t RegisterBuffer_(const sptr<NativeBuffer>& handle, const sptr<IRemoteObject> remote);

    static int32_t SetMetadata_(const sptr<NativeBuffer>& handle, uint32_t key, const std::vector<uint8_t>& value,
         const sptr<IRemoteObject> remote);

    static int32_t GetMetadata_(const sptr<NativeBuffer>& handle, uint32_t key, std::vector<uint8_t>& value,
         const sptr<IRemoteObject> remote);

    static int32_t ListMetadataKeys_(const sptr<NativeBuffer>& handle, std::vector<uint32_t>& keys,
         const sptr<IRemoteObject> remote);

    static int32_t EraseMetadataKey_(const sptr<NativeBuffer>& handle, uint32_t key, const sptr<IRemoteObject> remote);

    static int32_t GetVersion_(uint32_t& majorVer, uint32_t& minorVer, const sptr<IRemoteObject> remote);

    static int32_t Reconnect(sptr<OHOS::HDI::Display::Buffer::V1_1::MetadataProxy> proxy);

    sptr<IRemoteObject> GetCurrentRemote() {
        return isReconnected_ ? reconnectRemote_ : Remote();
    }

    bool isReconnected_;
    std::string serviceName_;
    sptr<IRemoteObject> servMgr_;
    sptr<OHOS::HDI::Display::Buffer::V1_1::MetadataProxy::IServiceManagerDeathRecipient> deathRecipient_;
    sptr<IRemoteObject> reconnectRemote_;
private:
    static inline BrokerDelegator<OHOS::HDI::Display::Buffer::V1_1::MetadataProxy> delegator_;
};

} // V1_1
} // Buffer
} // Display
} // HDI
} // OHOS

#endif // OHOS_HDI_DISPLAY_BUFFER_V1_1_METADATAPROXY_H
