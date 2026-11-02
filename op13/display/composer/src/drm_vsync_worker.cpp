/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#include "drm_vsync_worker.h"
#include <chrono>
#include "display_log.h"
#include "drm_device.h"

#include <atomic>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <xf86drm.h>
namespace OHOS {
namespace HDI {
namespace DISPLAY {
extern std::atomic<bool> g_hmosDisplayOff;
extern std::atomic<uint64_t> g_op13VsyncPeriodNs;
}
}

namespace HDI {
namespace DISPLAY {
DrmVsyncWorker::DrmVsyncWorker() {}

int32_t DrmVsyncWorker::Init(int fd)
{
    DISPLAY_CHK_RETURN((fd < 0), DISPLAY_FAILURE, DISPLAY_LOGE("the fd is invalid"));
    mDrmFd = fd;
    DISPLAY_LOGD("the drm fd is %{public}d", fd);
    /* must be set before the thread starts: the thread exits at once when
     * it sees mRunning == false (upstream race: no vsync for the whole boot) */
    mRunning = true;
    mThread = std::make_unique<std::thread>([this]() { WorkThread(); });
    DISPLAY_CHK_RETURN((mThread == nullptr), DISPLAY_FAILURE, DISPLAY_LOGE("can not create thread"));
    return DISPLAY_SUCCESS;
}

DrmVsyncWorker &DrmVsyncWorker::GetInstance()
{
    static DrmVsyncWorker instance;
    static std::once_flag once;
    std::call_once(once, [&]() {
        int ret = instance.Init(DrmDevice::GetDrmFd());
        if (ret != DISPLAY_SUCCESS) {
            DISPLAY_LOGE("Vsync Worker Init failed");
        }
    });
    return instance;
}

DrmVsyncWorker::~DrmVsyncWorker()
{
    DISPLAY_LOGD();
    {
        std::lock_guard<std::mutex> lg(mMutex);
        mRunning = false;
    }
    DISPLAY_LOGD();
    mCondition.notify_one();
    if (mThread != nullptr) {
        mThread->join();
    }
    DISPLAY_LOGD();
}

bool DrmVsyncWorker::WaitSignalAndCheckRuning()
{
    std::unique_lock<std::mutex> ul(mMutex);
    mCondition.wait(ul, [this]() { return (mEnable || !mRunning); });
    return mRunning;
}


/*
 * Vblank as a DRM event on our own card0 fd, waited with poll() and a short
 * timeout: a blocking drmWaitVBlank() never returns when the CRTC gives no
 * vblank interrupt (mode set in progress, panel off, splash hand-over), and
 * then render_service and every ArkUI app wait forever for their next vsync
 * ("ArkUI request vsync, but no vsync was received within 3 seconds").
 * Returns 0 when no real vblank arrived in time.
 */
uint64_t DrmVsyncWorker::WaitNextVBlank(unsigned int &sq)
{
    constexpr uint64_t SEC_TO_NSEC = 1000ULL * 1000 * 1000;
    constexpr uint64_t USEC_TO_NSEC = 1000;
    static int vfd = -2;
    static bool pending = false;
    if (vfd == -2) {
        vfd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
        DISPLAY_LOGI("vsync: own drm fd %{public}d", vfd);
    }
    int fd = vfd >= 0 ? vfd : mDrmFd;
    if (!pending) {
        drmVBlank vblank = {};
        vblank.request.type = drmVBlankSeqType(DRM_VBLANK_RELATIVE | DRM_VBLANK_EVENT);
        if (mCallBack != nullptr && mCallBack->GetPipe() == 1) {
            vblank.request.type = drmVBlankSeqType((int)(vblank.request.type) | (int)DRM_VBLANK_SECONDARY);
        }
        vblank.request.sequence = 1;
        vblank.request.signal = 0;
        if (drmWaitVBlank(fd, &vblank) != 0) {
            return 0;
        }
        pending = true;
    }
    struct pollfd pfd = {fd, POLLIN, 0};
    if (poll(&pfd, 1, 20) <= 0) {
        return 0;
    }
    char buf[1024];
    ssize_t n = read(fd, buf, sizeof(buf));
    uint64_t ts = 0;
    for (ssize_t off = 0; n > 0 && off + (ssize_t)sizeof(struct drm_event) <= n;) {
        struct drm_event *e = reinterpret_cast<struct drm_event *>(buf + off);
        if (e->length == 0) {
            break;
        }
        if (e->type == DRM_EVENT_VBLANK && off + (ssize_t)sizeof(struct drm_event_vblank) <= n) {
            struct drm_event_vblank *v = reinterpret_cast<struct drm_event_vblank *>(e);
            ts = (uint64_t)v->tv_sec * SEC_TO_NSEC + (uint64_t)v->tv_usec * USEC_TO_NSEC;
            sq = v->sequence;
            pending = false;
        }
        off += e->length;
    }
    return ts;
}


void DrmVsyncWorker::EnableVsync(bool enable)
{
    DISPLAY_LOGD();
    {
        std::lock_guard<std::mutex> lg(mMutex);
        mEnable = enable;
    }
    mCondition.notify_one();
}

void DrmVsyncWorker::WorkThread()
{
    DISPLAY_LOGD();
    unsigned int seq = 0;
    while (WaitSignalAndCheckRuning()) {
        // wait the vblank
        uint64_t time = WaitNextVBlank(seq);
        static uint64_t lastTs = 0;
        static unsigned int synthSeq = 0;
        static int synthCount = 0;
        if (time == 0) {
            /* no real vblank: with the panel off stay quiet, otherwise keep a
             * software vsync at the selected native rate on the grid of the last real one,
             * never a zero timestamp (render_service drops those) */
            if (g_hmosDisplayOff.load()) {
                usleep(16000);
                continue;
            }
            const uint64_t PERIOD = g_op13VsyncPeriodNs.load();
            struct timespec now;
            clock_gettime(CLOCK_MONOTONIC, &now);
            uint64_t n = (uint64_t)now.tv_sec * 1000000000ULL + (uint64_t)now.tv_nsec;
            if (lastTs == 0 || n - lastTs > 1000000000ULL) {
                lastTs = n;
            }
            uint64_t next = lastTs + PERIOD;
            if (next <= n) {
                next = lastTs + ((n - lastTs) / PERIOD + 1) * PERIOD;
            }
            struct timespec t = {(time_t)(next / 1000000000ULL), (long)(next % 1000000000ULL)};
            clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, nullptr);
            time = next;
            seq = ++synthSeq;
            if (synthCount++ % 600 == 0) {
                DISPLAY_LOGI("vsync: no hardware vblank, software vsync (%{public}d)", synthCount);
            }
        }
        lastTs = time;
        if (mCallBack != nullptr) {
            mCallBack->Vsync(seq, time);
        } else {
            DISPLAY_LOGE("the callbac is nullptr");
        }
    }
}

void DrmVsyncWorker::ReqesterVBlankCb(std::shared_ptr<VsyncCallBack> &cb)
{
    DISPLAY_LOGD();
    DISPLAY_CHK_RETURN_NOT_VALUE((cb == nullptr), DISPLAY_LOGE("the VBlankCallback is nullptr "));
    mCallBack = cb;
}
} // namespace OHOS
} // namespace HDI
} // namespace DISPLAY
