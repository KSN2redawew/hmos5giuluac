/*
 * Display buffer VDI for HarmonyOS on Redmi Note 10 Pro (sweet, SM7150).
 *
 * Allocates graphics buffers as DRM "dumb" buffers on the msm DRM device
 * (/dev/dri/card0) and hands them out as dma-buf fds inside a BufferHandle.
 * The same fds are scanned out by the composer (PRIME import + KMS) and
 * rendered into by Mesa's software rasterizer (kms_swrast, PRIME import).
 *
 * Interface: OpenHarmony 5.0.x IDisplayBufferVdi (vtable verified against the
 * HarmonyOS 5.0.5 vendor library).  License: Apache-2.0
 */
#define SWEET_LOG_TAG "OP13Gralloc"
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <map>
#include <mutex>
#include <vector>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <linux/dma-buf.h>
#include <xf86drm.h>
#include <drm.h>
#include <drm_mode.h>

#include "idisplay_buffer_vdi.h"
#include "v1_0/display_composer_type.h"
#include "sweet_log.h"

using namespace OHOS::HDI::Display::Buffer::V1_0;
using namespace OHOS::HDI::Display::Composer::V1_0;

namespace {
constexpr int32_t DISPLAY_SUCCESS_ = 0;
constexpr int32_t DISPLAY_FAILURE_ = -1;
constexpr int32_t DISPLAY_PARAM_ERR_ = -2;
constexpr int32_t DISPLAY_NOMEM_ = -6;
constexpr int32_t DISPLAY_NOT_SUPPORT_ = -5;

struct FormatInfo {
    uint32_t bitsPerPixel;   /* for the first plane, used for the dumb allocation */
    uint32_t heightNum;      /* total height = height * heightNum / heightDen */
    uint32_t heightDen;
};

bool GetFormatInfo(uint32_t format, FormatInfo &fi)
{
    switch (format) {
        case PIXEL_FMT_RGBX_8888:
        case PIXEL_FMT_RGBA_8888:
        case PIXEL_FMT_BGRX_8888:
        case PIXEL_FMT_BGRA_8888:
        case PIXEL_FMT_RGBA_1010102:
            fi = {32, 1, 1}; return true;
        case PIXEL_FMT_RGB_888:
            fi = {24, 1, 1}; return true;
        case PIXEL_FMT_RGB_565:
        case PIXEL_FMT_BGR_565:
        case PIXEL_FMT_RGBA_4444:
        case PIXEL_FMT_RGBX_4444:
        case PIXEL_FMT_BGRA_4444:
        case PIXEL_FMT_BGRX_4444:
        case PIXEL_FMT_RGBA_5551:
        case PIXEL_FMT_RGBX_5551:
        case PIXEL_FMT_BGRA_5551:
        case PIXEL_FMT_BGRX_5551:
        case PIXEL_FMT_YUYV_422_PKG:
        case PIXEL_FMT_UYVY_422_PKG:
        case PIXEL_FMT_YVYU_422_PKG:
        case PIXEL_FMT_VYUY_422_PKG:
        case PIXEL_FMT_YUV_422_I:
            fi = {16, 1, 1}; return true;
        case 38: /* PIXEL_FMT_BLOB: camera JPEG output, sized like a 4:2:0 frame */
        case PIXEL_FMT_YCBCR_420_SP:
        case PIXEL_FMT_YCRCB_420_SP:
        case PIXEL_FMT_YCBCR_420_P:
        case PIXEL_FMT_YCRCB_420_P:
            fi = {8, 3, 2}; return true;
        case PIXEL_FMT_YCBCR_422_SP:
        case PIXEL_FMT_YCRCB_422_SP:
        case PIXEL_FMT_YCBCR_422_P:
        case PIXEL_FMT_YCRCB_422_P:
            fi = {8, 2, 1}; return true;
        default:
            return false;
    }
}

int DrmFd()
{
    static int fd = -1;
    static std::once_flag once;
    std::call_once(once, [] {
        const char *nodes[] = {"/dev/dri/card0"};
        for (const char *n : nodes) {
            fd = open(n, O_RDWR | O_CLOEXEC);
            if (fd >= 0) {
                /* never keep DRM master: the composer needs it for KMS */
                if (drmIsMaster(fd)) {
                    drmDropMaster(fd);
                }
                SLOGI("using %{public}s fd %{public}d", n, fd);
                break;
            }
            SLOGE("open %{public}s failed: %{public}d", n, errno);
        }
    });
    return fd;
}

struct MetaKey {
    dev_t dev;
    ino_t ino;
    bool operator<(const MetaKey &o) const { return dev != o.dev ? dev < o.dev : ino < o.ino; }
};

class SweetBufferVdi : public IDisplayBufferVdi {
public:
    int32_t AllocMem(const AllocInfo &info, BufferHandle *&handle) const override
    {
        FormatInfo fi;
        if (info.width == 0 || info.height == 0 || !GetFormatInfo(info.format, fi)) {
            SLOGE("unsupported alloc %{public}ux%{public}u fmt %{public}u", info.width, info.height, info.format);
            return DISPLAY_NOT_SUPPORT_;
        }
        int drm = DrmFd();
        if (drm < 0) {
            return DISPLAY_FAILURE_;
        }
        /* width aligned to 64 px (Adreno a6xx needs >= 64-byte pitch alignment), height covers all planes */
        uint32_t alignedW = (info.width + 63) & ~63u;
        uint32_t allocH = (info.height * fi.heightNum + fi.heightDen - 1) / fi.heightDen;
        struct drm_mode_create_dumb create = {};
        create.width = alignedW;
        create.height = allocH;
        create.bpp = fi.bitsPerPixel;
        if (drmIoctl(drm, DRM_IOCTL_MODE_CREATE_DUMB, &create) != 0) {
            SLOGE("CREATE_DUMB %{public}ux%{public}u bpp %{public}u failed: %{public}d", alignedW, allocH,
                fi.bitsPerPixel, errno);
            return DISPLAY_NOMEM_;
        }
        int dmaFd = -1;
        int ret = drmPrimeHandleToFD(drm, create.handle, DRM_CLOEXEC | DRM_RDWR, &dmaFd);
        struct drm_mode_destroy_dumb destroy = {create.handle};
        drmIoctl(drm, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy); /* the dma-buf keeps the object alive */
        if (ret != 0 || dmaFd < 0) {
            SLOGE("PRIME export failed: %{public}d", errno);
            return DISPLAY_FAILURE_;
        }
        auto *bh = static_cast<BufferHandle *>(calloc(1, sizeof(BufferHandle)));
        if (bh == nullptr) {
            close(dmaFd);
            return DISPLAY_NOMEM_;
        }
        bh->fd = dmaFd;
        bh->width = static_cast<int32_t>(info.width);
        bh->height = static_cast<int32_t>(info.height);
        bh->stride = static_cast<int32_t>(create.pitch);
        bh->size = static_cast<int32_t>(create.size);
        bh->format = static_cast<int32_t>(info.format);
        bh->usage = info.usage;
        bh->virAddr = nullptr;
        bh->phyAddr = 0;
        bh->reserveFds = 0;
        bh->reserveInts = 0;
        handle = bh;
        SLOGI("alloc %{public}ux%{public}u fmt %{public}u -> fd %{public}d stride %{public}u size %{public}llu",
            info.width, info.height, info.format, dmaFd, create.pitch, (unsigned long long)create.size);
        return DISPLAY_SUCCESS_;
    }

    void FreeMem(const BufferHandle &handle) const override
    {
        auto *bh = const_cast<BufferHandle *>(&handle);
        if (bh->virAddr != nullptr) {
            munmap(bh->virAddr, bh->size);
            bh->virAddr = nullptr;
        }
        if (bh->fd >= 0) {
            close(bh->fd);
        }
        for (uint32_t i = 0; i < bh->reserveFds; i++) {
            if (bh->reserve[i] >= 0) {
                close(bh->reserve[i]);
            }
        }
        free(bh);
    }

    void *Mmap(const BufferHandle &handle) const override
    {
        auto *bh = const_cast<BufferHandle *>(&handle);
        if (bh->virAddr != nullptr) {
            return bh->virAddr;
        }
        void *p = mmap(nullptr, bh->size, PROT_READ | PROT_WRITE, MAP_SHARED, bh->fd, 0);
        if (p == MAP_FAILED) {
            SLOGE("mmap fd %{public}d size %{public}d failed: %{public}d", bh->fd, bh->size, errno);
            return nullptr;
        }
        bh->virAddr = p;
        return p;
    }

    int32_t Unmap(const BufferHandle &handle) const override
    {
        auto *bh = const_cast<BufferHandle *>(&handle);
        if (bh->virAddr == nullptr) {
            return DISPLAY_PARAM_ERR_;
        }
        munmap(bh->virAddr, bh->size);
        bh->virAddr = nullptr;
        return DISPLAY_SUCCESS_;
    }

    int32_t FlushCache(const BufferHandle &handle) const override
    {
        struct dma_buf_sync s = {DMA_BUF_SYNC_END | DMA_BUF_SYNC_RW};
        ioctl(handle.fd, DMA_BUF_IOCTL_SYNC, &s);
        return DISPLAY_SUCCESS_;
    }

    int32_t InvalidateCache(const BufferHandle &handle) const override
    {
        struct dma_buf_sync s = {DMA_BUF_SYNC_START | DMA_BUF_SYNC_RW};
        ioctl(handle.fd, DMA_BUF_IOCTL_SYNC, &s);
        return DISPLAY_SUCCESS_;
    }

    int32_t IsSupportedAlloc(const std::vector<VerifyAllocInfo> &infos, std::vector<bool> &supporteds) const override
    {
        supporteds.clear();
        for (const auto &i : infos) {
            FormatInfo fi;
            supporteds.push_back(GetFormatInfo(i.format, fi));
        }
        return DISPLAY_SUCCESS_;
    }

    int32_t RegisterBuffer(const BufferHandle &handle) override
    {
        return DISPLAY_SUCCESS_;
    }

    /* Metadata is kept per process, keyed by the dma-buf inode. */
    int32_t SetMetadata(const BufferHandle &handle, uint32_t key, const std::vector<uint8_t> &value) override
    {
        MetaKey k;
        if (!KeyOf(handle, k)) {
            return DISPLAY_PARAM_ERR_;
        }
        std::lock_guard<std::mutex> l(mutex_);
        meta_[k][key] = value;
        return DISPLAY_SUCCESS_;
    }

    int32_t GetMetadata(const BufferHandle &handle, uint32_t key, std::vector<uint8_t> &value) override
    {
        MetaKey k;
        if (!KeyOf(handle, k)) {
            return DISPLAY_PARAM_ERR_;
        }
        std::lock_guard<std::mutex> l(mutex_);
        auto it = meta_.find(k);
        if (it == meta_.end() || it->second.find(key) == it->second.end()) {
            return DISPLAY_NOT_SUPPORT_;
        }
        value = it->second[key];
        return DISPLAY_SUCCESS_;
    }

    int32_t ListMetadataKeys(const BufferHandle &handle, std::vector<uint32_t> &keys) override
    {
        keys.clear();
        MetaKey k;
        if (!KeyOf(handle, k)) {
            return DISPLAY_PARAM_ERR_;
        }
        std::lock_guard<std::mutex> l(mutex_);
        auto it = meta_.find(k);
        if (it != meta_.end()) {
            for (auto &kv : it->second) {
                keys.push_back(kv.first);
            }
        }
        return DISPLAY_SUCCESS_;
    }

    int32_t EraseMetadataKey(const BufferHandle &handle, uint32_t key) override
    {
        MetaKey k;
        if (!KeyOf(handle, k)) {
            return DISPLAY_PARAM_ERR_;
        }
        std::lock_guard<std::mutex> l(mutex_);
        auto it = meta_.find(k);
        if (it != meta_.end()) {
            it->second.erase(key);
        }
        return DISPLAY_SUCCESS_;
    }

    int32_t GetImageLayout(const BufferHandle &handle, OHOS::HDI::Display::Buffer::V1_2::ImageLayout &layout) const override
    {
        return DISPLAY_NOT_SUPPORT_;
    }

private:
    static bool KeyOf(const BufferHandle &h, MetaKey &k)
    {
        struct stat st;
        if (h.fd < 0 || fstat(h.fd, &st) != 0) {
            return false;
        }
        k = {st.st_dev, st.st_ino};
        return true;
    }
    std::mutex mutex_;
    std::map<MetaKey, std::map<uint32_t, std::vector<uint8_t>>> meta_;
};
} // namespace

extern "C" IDisplayBufferVdi *CreateDisplayBufferVdi()
{
    SLOGI("sweet display buffer VDI (DRM dumb + PRIME)");
    return new SweetBufferVdi();
}

extern "C" void DestroyDisplayBufferVdi(IDisplayBufferVdi *vdi)
{
    delete vdi;
}
