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

#include "hdi_drm_layer.h"
#include <mutex>
#include <vector>
#include <sys/stat.h>
#include <cinttypes>
#include <cerrno>
#include "display_log.h"
#include "drm_device.h"

namespace OHOS {
namespace HDI {
namespace DISPLAY {
DrmGemBuffer::DrmGemBuffer(int drmFd, HdiLayerBuffer &hdl) : mDrmFd(drmFd)
{
    DISPLAY_LOGD();
    Init(mDrmFd, hdl);
}

/*
 * sweet: render_service cycles through the same 3-4 buffers. Importing every
 * frame (PRIME import + AddFB2, RmFB + GEM close two frames later) costs time
 * on the commit path, so framebuffers are cached by dma-buf identity and only
 * released when the cache is full.
 */
namespace {
struct FbCacheEntry {
    uint64_t ino;
    uint32_t w, h, stride, fmt;
    uint32_t gem, fb;
    int refs;
    uint64_t lastUse;
};
std::mutex g_fbCacheLock;
std::vector<FbCacheEntry> g_fbCache;
uint64_t g_fbUse;
constexpr size_t FB_CACHE_MAX = 8;

void ReleaseEntry(int drmFd, const FbCacheEntry &e)
{
    drmModeRmFB(drmFd, e.fb);
    struct drm_gem_close gemClose = { 0 };
    gemClose.handle = e.gem;
    drmIoctl(drmFd, DRM_IOCTL_GEM_CLOSE, &gemClose);
}
} // namespace

void DrmGemBuffer::Init(int drmFd, HdiLayerBuffer &hdl)
{
    int ret;
    const int MAX_COUNT = 4;
    uint32_t pitches[MAX_COUNT] = {0};
    uint32_t gemHandles[MAX_COUNT] = {0};
    uint32_t offsets[MAX_COUNT] = {0};
    DISPLAY_CHK_RETURN_NOT_VALUE((drmFd < 0), DISPLAY_LOGE("can not init drmfd %{public}d", drmFd));
    mDrmFormat = DrmDevice::ConvertToDrmFormat(static_cast<PixelFormat>(hdl.GetFormat()));
    struct stat st;
    uint64_t ino = (fstat(hdl.GetFb(), &st) == 0) ? (uint64_t)st.st_ino : 0;

    std::lock_guard<std::mutex> lock(g_fbCacheLock);
    if (ino != 0) {
        for (auto &e : g_fbCache) {
            if (e.ino == ino && e.w == (uint32_t)hdl.GetWight() && e.h == (uint32_t)hdl.GetHeight() &&
                e.stride == (uint32_t)hdl.GetStride() && e.fmt == mDrmFormat) {
                e.refs++;
                e.lastUse = ++g_fbUse;
                mGemHandle = e.gem;
                mFdId = e.fb;
                mCached = true;
                return;
            }
        }
    }
    ret = drmPrimeFDToHandle(drmFd, hdl.GetFb(), &mGemHandle);
    DISPLAY_CHK_RETURN_NOT_VALUE((ret != 0), DISPLAY_LOGE("can not get handle errno %{public}d", errno));

    pitches[0] = hdl.GetStride();
    gemHandles[0] = mGemHandle;
    offsets[0] = 0;
    ret = drmModeAddFB2(drmFd, hdl.GetWight(), hdl.GetHeight(), mDrmFormat, gemHandles, pitches, offsets, &mFdId, 0);
    DISPLAY_CHK_RETURN_NOT_VALUE((ret != 0), DISPLAY_LOGE("can not add fb errno %{public}d", errno));
    if (ino == 0) {
        return;
    }
    if (g_fbCache.size() >= FB_CACHE_MAX) {
        /* drop the least recently used framebuffer nobody holds */
        size_t victim = g_fbCache.size();
        for (size_t i = 0; i < g_fbCache.size(); i++) {
            if (g_fbCache[i].refs == 0 && (victim == g_fbCache.size() ||
                g_fbCache[i].lastUse < g_fbCache[victim].lastUse)) {
                victim = i;
            }
        }
        if (victim == g_fbCache.size()) {
            return; /* everything busy: this one stays uncached */
        }
        if (g_fbCache[victim].gem != mGemHandle) {
            ReleaseEntry(drmFd, g_fbCache[victim]);
        } else {
            drmModeRmFB(drmFd, g_fbCache[victim].fb);
        }
        g_fbCache.erase(g_fbCache.begin() + victim);
    }
    g_fbCache.push_back(FbCacheEntry { ino, (uint32_t)hdl.GetWight(), (uint32_t)hdl.GetHeight(),
        (uint32_t)hdl.GetStride(), mDrmFormat, mGemHandle, mFdId, 1, ++g_fbUse });
    mCached = true;
}

DrmGemBuffer::~DrmGemBuffer()
{
    DISPLAY_LOGD();
    if (mCached) {
        std::lock_guard<std::mutex> lock(g_fbCacheLock);
        for (auto &e : g_fbCache) {
            if (e.fb == mFdId) {
                e.refs--;
                break;
            }
        }
        return;
    }
    if (mFdId) {
        if (drmModeRmFB(mDrmFd, mFdId)) {
            DISPLAY_LOGE("can not free fdid %{public}d errno %{public}d", mFdId, errno);
        }
    }

    if (mGemHandle) {
        struct drm_gem_close gemClose = { 0 };
        gemClose.handle = mGemHandle;
        if (drmIoctl(mDrmFd, DRM_IOCTL_GEM_CLOSE, &gemClose)) {
            DISPLAY_LOGD("can not free gem handle %{public}d errno : %{public}d", mGemHandle, errno);
        }
    }
}

bool DrmGemBuffer::IsValid()
{
    DISPLAY_LOGD();
    return (mGemHandle != INVALID_DRM_ID) && (mFdId != INVALID_DRM_ID);
}

DrmGemBuffer *HdiDrmLayer::GetGemBuffer()
{
    DISPLAY_LOGD();
    std::unique_ptr<DrmGemBuffer> ptr = std::make_unique<DrmGemBuffer>(DrmDevice::GetDrmFd(), *GetCurrentBuffer());
    mLastBuffer = std::move(mCurrentBuffer);
    mCurrentBuffer = std::move(ptr);
    return mCurrentBuffer.get();
}
} // namespace OHOS
} // namespace HDI
} // namespace DISPLAY
