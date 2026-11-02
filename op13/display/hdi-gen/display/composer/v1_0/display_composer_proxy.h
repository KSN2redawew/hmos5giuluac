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

#ifndef OHOS_HDI_DISPLAY_COMPOSER_V1_0_DISPLAYCOMPOSERPROXY_H
#define OHOS_HDI_DISPLAY_COMPOSER_V1_0_DISPLAYCOMPOSERPROXY_H

#include "v1_0/idisplay_composer.h"
#include <unistd.h>
#include <iproxy_broker.h>

namespace OHOS {
namespace HDI {
namespace Display {
namespace Composer {
namespace V1_0 {

class DisplayComposerProxy : public IProxyBroker<OHOS::HDI::Display::Composer::V1_0::IDisplayComposer> {
public:
    class IServiceManagerDeathRecipient : public IRemoteObject::DeathRecipient {
    public:
        IServiceManagerDeathRecipient(wptr<OHOS::HDI::Display::Composer::V1_0::DisplayComposerProxy> proxy) : proxy_(proxy) {}
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
                    result = OHOS::HDI::Display::Composer::V1_0::DisplayComposerProxy::Reconnect(proxy);
                }
                --currentTime;
            } while (result != HDF_SUCCESS && currentTime >0);
        }
    private:
        wptr<OHOS::HDI::Display::Composer::V1_0::DisplayComposerProxy> proxy_;
    };

    explicit DisplayComposerProxy(const sptr<IRemoteObject>& remote) : IProxyBroker<OHOS::HDI::Display::Composer::V1_0::IDisplayComposer>(remote) {
        reconnectRemote_ = nullptr;
        servMgr_ = nullptr;
        deathRecipient_ = nullptr;
        isReconnected_ = false;
    }
    virtual ~DisplayComposerProxy() {
        if (servMgr_ != nullptr && deathRecipient_ != nullptr) {
            servMgr_->RemoveDeathRecipient(deathRecipient_);
        }
    }

    inline bool IsProxy() override
    {
        return true;
    }

    int32_t RegHotPlugCallback(const sptr<OHOS::HDI::Display::Composer::V1_0::IHotPlugCallback>& cb) override;

    int32_t SetClientBufferCacheCount(uint32_t devId, uint32_t count) override;

    int32_t RegDisplayVBlankCallback(uint32_t devId,
         const sptr<OHOS::HDI::Display::Composer::V1_0::IVBlankCallback>& cb) override;

    int32_t GetDisplayCapability(uint32_t devId, OHOS::HDI::Display::Composer::V1_0::DisplayCapability& info) override;

    int32_t GetDisplaySupportedModes(uint32_t devId,
         std::vector<OHOS::HDI::Display::Composer::V1_0::DisplayModeInfo>& modes) override;

    int32_t GetDisplayMode(uint32_t devId, uint32_t& modeId) override;

    int32_t SetDisplayMode(uint32_t devId, uint32_t modeId) override;

    int32_t GetDisplayPowerStatus(uint32_t devId, OHOS::HDI::Display::Composer::V1_0::DispPowerStatus& status) override;

    int32_t SetDisplayPowerStatus(uint32_t devId, OHOS::HDI::Display::Composer::V1_0::DispPowerStatus status) override;

    int32_t GetDisplayBacklight(uint32_t devId, uint32_t& level) override;

    int32_t SetDisplayBacklight(uint32_t devId, uint32_t level) override;

    int32_t SetDisplayVsyncEnabled(uint32_t devId, bool enabled) override;

    int32_t CreateLayer(uint32_t devId, const OHOS::HDI::Display::Composer::V1_0::LayerInfo& layerInfo,
         uint32_t cacheCount, uint32_t& layerId) override;

    int32_t DestroyLayer(uint32_t devId, uint32_t layerId) override;

    int32_t SetDisplayClientCrop(uint32_t devId, const OHOS::HDI::Display::Composer::V1_0::IRect& rect) override;

    int32_t GetDisplayReleaseFence(uint32_t devId, std::vector<uint32_t>& layers,
         std::vector<sptr<OHOS::HDI::Display::HdifdParcelable>>& fences) override;

    int32_t CreateVirtualDisplay(uint32_t width, uint32_t height, int32_t& format, uint32_t& devId) override;

    int32_t DestroyVirtualDisplay(uint32_t devId) override;

    int32_t SetVirtualDisplayBuffer(uint32_t devId, const sptr<NativeBuffer>& buffer,
         const sptr<OHOS::HDI::Display::HdifdParcelable>& fence) override;

    int32_t SetDisplayProperty(uint32_t devId, uint32_t id, uint64_t value) override;

    int32_t GetDisplayProperty(uint32_t devId, uint32_t id, uint64_t& value) override;

    int32_t InitCmdRequest(const std::shared_ptr<SharedMemQueue<int32_t>>& request) override;

    int32_t CmdRequest(uint32_t inEleCnt, const std::vector<OHOS::HDI::Display::Composer::V1_0::HdifdInfo>& inFds,
         uint32_t& outEleCnt, std::vector<OHOS::HDI::Display::Composer::V1_0::HdifdInfo>& outFds) override;

    int32_t GetCmdReply(std::shared_ptr<SharedMemQueue<int32_t>>& reply) override;

    int32_t GetVersion(uint32_t& majorVer, uint32_t& minorVer) override;

    static int32_t RegHotPlugCallback_(const sptr<OHOS::HDI::Display::Composer::V1_0::IHotPlugCallback>& cb,
         const sptr<IRemoteObject> remote);

    static int32_t SetClientBufferCacheCount_(uint32_t devId, uint32_t count, const sptr<IRemoteObject> remote);

    static int32_t RegDisplayVBlankCallback_(uint32_t devId,
         const sptr<OHOS::HDI::Display::Composer::V1_0::IVBlankCallback>& cb, const sptr<IRemoteObject> remote);

    static int32_t GetDisplayCapability_(uint32_t devId, OHOS::HDI::Display::Composer::V1_0::DisplayCapability& info,
         const sptr<IRemoteObject> remote);

    static int32_t GetDisplaySupportedModes_(uint32_t devId,
         std::vector<OHOS::HDI::Display::Composer::V1_0::DisplayModeInfo>& modes, const sptr<IRemoteObject> remote);

    static int32_t GetDisplayMode_(uint32_t devId, uint32_t& modeId, const sptr<IRemoteObject> remote);

    static int32_t SetDisplayMode_(uint32_t devId, uint32_t modeId, const sptr<IRemoteObject> remote);

    static int32_t GetDisplayPowerStatus_(uint32_t devId, OHOS::HDI::Display::Composer::V1_0::DispPowerStatus& status,
         const sptr<IRemoteObject> remote);

    static int32_t SetDisplayPowerStatus_(uint32_t devId, OHOS::HDI::Display::Composer::V1_0::DispPowerStatus status,
         const sptr<IRemoteObject> remote);

    static int32_t GetDisplayBacklight_(uint32_t devId, uint32_t& level, const sptr<IRemoteObject> remote);

    static int32_t SetDisplayBacklight_(uint32_t devId, uint32_t level, const sptr<IRemoteObject> remote);

    static int32_t SetDisplayVsyncEnabled_(uint32_t devId, bool enabled, const sptr<IRemoteObject> remote);

    static int32_t CreateLayer_(uint32_t devId, const OHOS::HDI::Display::Composer::V1_0::LayerInfo& layerInfo,
         uint32_t cacheCount, uint32_t& layerId, const sptr<IRemoteObject> remote);

    static int32_t DestroyLayer_(uint32_t devId, uint32_t layerId, const sptr<IRemoteObject> remote);

    static int32_t SetDisplayClientCrop_(uint32_t devId, const OHOS::HDI::Display::Composer::V1_0::IRect& rect,
         const sptr<IRemoteObject> remote);

    static int32_t GetDisplayReleaseFence_(uint32_t devId, std::vector<uint32_t>& layers,
         std::vector<sptr<OHOS::HDI::Display::HdifdParcelable>>& fences, const sptr<IRemoteObject> remote);

    static int32_t CreateVirtualDisplay_(uint32_t width, uint32_t height, int32_t& format, uint32_t& devId,
         const sptr<IRemoteObject> remote);

    static int32_t DestroyVirtualDisplay_(uint32_t devId, const sptr<IRemoteObject> remote);

    static int32_t SetVirtualDisplayBuffer_(uint32_t devId, const sptr<NativeBuffer>& buffer,
         const sptr<OHOS::HDI::Display::HdifdParcelable>& fence, const sptr<IRemoteObject> remote);

    static int32_t SetDisplayProperty_(uint32_t devId, uint32_t id, uint64_t value, const sptr<IRemoteObject> remote);

    static int32_t GetDisplayProperty_(uint32_t devId, uint32_t id, uint64_t& value, const sptr<IRemoteObject> remote);

    static int32_t InitCmdRequest_(const std::shared_ptr<SharedMemQueue<int32_t>>& request,
         const sptr<IRemoteObject> remote);

    static int32_t CmdRequest_(uint32_t inEleCnt,
         const std::vector<OHOS::HDI::Display::Composer::V1_0::HdifdInfo>& inFds, uint32_t& outEleCnt, std::vector<OHOS::HDI::Display::Composer::V1_0::HdifdInfo>& outFds, const sptr<IRemoteObject> remote);

    static int32_t GetCmdReply_(std::shared_ptr<SharedMemQueue<int32_t>>& reply, const sptr<IRemoteObject> remote);

    static int32_t GetVersion_(uint32_t& majorVer, uint32_t& minorVer, const sptr<IRemoteObject> remote);

    static int32_t Reconnect(sptr<OHOS::HDI::Display::Composer::V1_0::DisplayComposerProxy> proxy);

    sptr<IRemoteObject> GetCurrentRemote() {
        return isReconnected_ ? reconnectRemote_ : Remote();
    }

    bool isReconnected_;
    std::string serviceName_;
    sptr<IRemoteObject> servMgr_;
    sptr<OHOS::HDI::Display::Composer::V1_0::DisplayComposerProxy::IServiceManagerDeathRecipient> deathRecipient_;
    sptr<IRemoteObject> reconnectRemote_;
private:
    static inline BrokerDelegator<OHOS::HDI::Display::Composer::V1_0::DisplayComposerProxy> delegator_;
};

} // V1_0
} // Composer
} // Display
} // HDI
} // OHOS

#endif // OHOS_HDI_DISPLAY_COMPOSER_V1_0_DISPLAYCOMPOSERPROXY_H
