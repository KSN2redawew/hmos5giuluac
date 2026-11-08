# HarmonyOS NEXT 5.1 port for OnePlus Ace 5 (giuliac / PKG110)

Experimental **HarmonyOS NEXT 5.1 DSU port for OnePlus Ace 5 (OP5D2BL1 / PKG110)**,
based on the Snapdragon 8 Gen 3 (`pineapple` / SM8650) stock kernel, stock vendor base,
and matching OnePlus Ace 5 DLKM images, booting the native HarmonyOS NEXT 5 userspace
(SceneBoard, SuperLauncher, OpenHarmony 5.0.3+ ABI) through Android Dynamic System Updates (DSU).

---

## 1. Device Profile & Hardware Specs

| Property | Value |
| --- | --- |
| **Model** | OnePlus Ace 5 (`PKG110`) |
| **Internal Device** | `OP5D2BL1` |
| **SoC / Platform** | Qualcomm Snapdragon 8 Gen 3 (`pineapple` / SM8650) |
| **Stock Kernel** | Linux 6.1.141-android14-11-o-gb0878e0704d9 |
| **Android Base** | Android 17 / SDK 37 / Security patch `2026-09-01` |
| **Display Panel** | `card0-DSI-1`, 1264x2780 px, 560 dpi |
| **Refresh Rates** | 60 Hz, 90 Hz, 120 Hz |
| **Camera / Audio** | Stock vendor HALs & DLKM drivers |

---

## 2. DSU Multipart Package Layout

The generated package is **`hmos5-giuliac-dsu.zip`** (also accessible at `F:\fowa\hmos5-giuliac-dsu.zip`).
It contains 8 separate signed raw filesystem partition images:

| Image | Description |
| --- | --- |
| `system.img` | HarmonyOS NEXT 5.1 native framework with Ace 5 PID-1 DSU bootstrap handoff, storage daemon bind shim, Ace 5 display geometry, and setup bypass. |
| `vendor.img` | Stock OnePlus Ace 5 vendor base with selected native HarmonyOS runtime additions (display buffer/composer VDI, software Mesa). |
| `vendor_dlkm.img` | Current OnePlus Ace 5 stock vendor kernel modules (432 modules, unchanged EROFS). |
| `system_dlkm.img` | Current OnePlus Ace 5 stock GKI modules (120 modules, unchanged EROFS). |
| `sys_prod.img` | HarmonyOS NEXT 5 sys_prod (UI resources and window manager configurations). |
| `chip_prod.img` | HarmonyOS NEXT 5 chip_prod (HDF default configurations). |
| `cust.img` | HarmonyOS NEXT 5 cust filesystem. |
| `patch_hw.img` | HarmonyOS NEXT 5 patch filesystem (named for native `/patch_hw` mountpoint). |

*Note: The device's existing `/odm` partition is reused read-only directly from the host storage; no `odm.img` is required in the package.*

---

## 3. Architecture & Boot Flow

1. **Android First Stage**:
   Mounts all eight DSU partitions to `system_gsi`, `vendor_gsi`, `vendor_dlkm_gsi`, `system_dlkm_gsi`, `chip_prod_gsi`, `sys_prod_gsi`, `cust_gsi`, `patch_hw_gsi`.
2. **PID 1 Handoff (`giuliac/dsu_bootstrap.c`)**:
   - Takes control upon `selinux_setup`.
   - Creates a new root at `/mnt/hmos-root` (tmpfs).
   - Verifies the DSU partition mappings and binds them into the new root.
   - Binds host `/odm` read-only.
   - Pins and formats `userdata_gsi` (ext4) to `/data` if blank; preserves nonblank data.
   - Loads the Ace 5 stock driver modules (`msm_drm`, etc.) into the running kernel.
   - Allocates private BinderFS instances (`binder`, `hwbinder`, `vndbinder`).
   - Populates character device nodes (`/dev/dri/card0`, `/dev/kgsl-3d0`, `/dev/input/event*`, `/dev/dma_heap/*`).
   - Attaches cgroup BPF device filter blocking physical block device access.
   - Chroots into `/mnt/hmos-root` and starts `/system/bin/init --second-stage 0` with `OHOS_MESA_SOFTWARE=true`.
3. **Storage Shim (`giuliac/local_storage.c`)**:
   Intercepts `storage_daemon` to emulate `hmdfs` and `sharefs` using Linux private bind mounts without requiring out-of-tree kernel patches.

---

## 4. How to Install & Test

1. Ensure bootloader is unlocked.
2. Install **[DSU Sideloader](https://github.com/VegaBobo/DSU-Sideloader)** on the phone.
3. Push or copy `hmos5-giuliac-dsu.zip` to device storage:
   ```sh
   adb push F:\fowa\hmos5-giuliac-dsu.zip /sdcard/Download/
   ```
4. Open DSU Sideloader, select `hmos5-giuliac-dsu.zip`, allocate at least 16 GB for DSU userdata, and install.
5. In notification drawer, choose to restart into the DSU system.
6. Diagnostic logs will be appended to `/data/hmos-port/bootstrap.log` in the DSU partition.
