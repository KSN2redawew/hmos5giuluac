// SPDX-License-Identifier: Apache-2.0
// Execute the real candidate classes against synthetic connector timings.
// No DRM fd, device access, display allocation or modesetting is used.
#include <cassert>
#include <atomic>
#include <cstdio>
#include <memory>
#include <vector>
#include "drm_display.h"
#include "drm_device.h"
#include "display_composer_vdi_impl.h"

namespace OHOS::HDI::DISPLAY {
extern std::atomic<uint64_t> g_op13VsyncPeriodNs;
}

using namespace OHOS::HDI::DISPLAY;

int main()
{
    drmModeModeInfo timings[4] = {};
    const unsigned rates[] = {120, 60, 90, 120};
    for (unsigned i = 0; i < 4; ++i) {
        timings[i].hdisplay = 1440;
        timings[i].vdisplay = 3168;
        timings[i].vrefresh = rates[i];
    }
    timings[0].type = DRM_MODE_TYPE_PREFERRED;
    drmModeConnector raw = {};
    raw.connector_id = 71;
    raw.connector_type = DRM_MODE_CONNECTOR_DSI;
    raw.connection = DRM_MODE_CONNECTED;
    raw.count_modes = 4;
    raw.modes = timings;
    FdPtr unusedFd;
    auto connector = std::make_shared<DrmConnector>(raw, unusedFd);
    drmModeCrtc crtcInfo = {};
    crtcInfo.crtc_id = 88;
    auto crtc = std::make_shared<DrmCrtc>(&crtcInfo, 0);
    DrmDisplay display(connector, crtc, nullptr);

    uint32_t count = 0;
    assert(display.GetDisplaySupportedModes(&count, nullptr) == 0 && count == 4);
    std::vector<DisplayModeInfo> modes(count);
    assert(display.GetDisplaySupportedModes(&count, modes.data()) == 0);
    for (unsigned i = 0; i < 4; ++i) {
        assert(modes[i].id == i && modes[i].freshRate == rates[i]);
        assert(modes[i].width == 1440 && modes[i].height == 3168);
    }
    DisplayModeInfo tooSmall = {};
    tooSmall.id = 12345;
    count = 1;
    assert(display.GetDisplaySupportedModes(&count, &tooSmall) != 0);
    assert(count == 4 && tooSmall.id == 12345);
    assert(display.GetDisplaySupportedModes(nullptr, nullptr) != 0);
    assert(display.GetDisplayMode(nullptr) != 0);
    uint32_t active;
    assert(display.GetDisplayMode(&active) != 0);

    for (unsigned i = 0; i < 4; ++i) {
        assert(display.SetDisplayMode(i) == 0);
        assert(display.GetDisplayMode(&active) == 0 && active == i);
        assert(crtc->GetActiveModeId() == static_cast<int32_t>(i));
        assert(g_op13VsyncPeriodNs.load() == (1000000000ULL + rates[i] / 2) / rates[i]);
    }
    assert(display.SetDisplayMode(99) != 0);
    assert(display.SetDisplayMode(UINT32_MAX) != 0);
    assert(display.GetDisplayMode(&active) == 0 && active == 3);
    assert(crtc->SetActivieMode(-1) != 0);

    // Construct/destroy the exported interface without invoking device startup.
    auto *interface = OHOS::HDI::Display::Composer::CreateComposerVdiV1_1();
    assert(interface != nullptr);
    uint32_t frames = 99, vsyncs = 99;
    interface->GetHardwareCursorStats(0, frames, vsyncs);
    OHOS::HDI::Display::Composer::DestroyComposerVdiV1_1(interface);
    puts("PASS native mode IDs, 60/90/120 Hz timing, bounded mode enumeration, invalid-mode rejection, interface construction");
    return 0;
}
