# HarmonyOS NEXT 5 Ports (OnePlus Ace 5 & OnePlus 13)

Experimental **HarmonyOS NEXT 5.1 DSU ports for OnePlus Ace 5 (`giuliac` / `PKG110`) and OnePlus 13 (`palas` / `dodge`)**, using stock vendor bases, matching DLKM images, and separate partition images installed through Android Dynamic System Updates (DSU).

The port targets the native HarmonyOS NEXT framework, SceneBoard, SuperLauncher, and OpenHarmony 5.0.3+ ABI with software Mesa rendering and guarded first-stage PID-1 handoff.

## Supported Devices

- **OnePlus Ace 5 (`giuliac` / `PKG110` / `OP5D2BL1`)**: See **[giuliac/README.md](giuliac/README.md)**.
  - SoC: Snapdragon 8 Gen 3 (`pineapple` / SM8650)
  - Display: 1264x2780, 560 DPI, 120Hz
  - Package: `hmos5-giuliac-dsu.zip`
- **OnePlus 13 (`palas` / `dodge`)**: See **[op13/README.md](op13/README.md)**.
  - SoC: Snapdragon 8 Elite (`sun` / SM8750)
  - Package: `hmos5-op13-dsu.zip`

## Status

**The candidate has passed offline validation. Boot on OP13 remains unverified.**

- Separate partition images and the DSU ZIP have been built and checked.
- OP13 vendor contents, permissions, capabilities and symlinks are preserved.
- The kernel is unchanged; initial rendering uses software Mesa.
- Display hardware, touch, GPU acceleration, connectivity and full framework
  startup still need device testing.
- The stale input HDF-host entry is removed; the existing Linux userspace
  input backend has passed an isolated ARM64 initialization test. Phone touch
  events and permissions remain unverified.

## Package layout

| Image | Base |
| --- | --- |
| `system.img` | HMOS5 with the OP13 boot handoff and runtime adaptations |
| `vendor.img` | OP13 stock vendor with selected native HMOS additions |
| `vendor_dlkm.img` | Current OP13 vendor kernel modules, unchanged |
| `system_dlkm.img` | Current OP13 GKI modules, unchanged |
| `sys_prod.img` | HMOS5 sys_prod |
| `chip_prod.img` | HMOS5 chip_prod |
| `cust.img` | HMOS5 cust |
| `patch_hw.img` | HMOS5 patch |

The eight images are packaged together as `hmos5-op13-dsu.zip`. The phone's
mounted stock ODM EROFS is reused read-only; `odm.img` is omitted. Output copies receive
local development AVB signatures; device acceptance remains unverified.
Bootloader, physical vbmeta, boot, vendor_boot, dtbo and firmware-partition
images are outside this DSU package. OP13 anti-rollback constraints still apply.

## Build and test

See **[op13/README.md](op13/README.md)** for the build workflow, required local
inputs, validation scope, DSU installation steps and boot-log collection.

The OP13 implementation is in [`op13/`](op13/). Its main tools are:

- [`prepare_vendor.py`](op13/prepare_vendor.py): select native HMOS runtime additions.
- [`prepare_dlkm.py`](op13/prepare_dlkm.py): inspect matching stock module images.
- [`extend_with_dlkm.py`](op13/extend_with_dlkm.py): update an existing candidate with DLKM and omit ODM.
- [`build_multipart.py`](op13/build_multipart.py): build separate partition images and a DSU ZIP.
- [`verify_multipart.py`](op13/verify_multipart.py): check packed contents and input preservation.
- [`validate_components.py`](op13/validate_components.py): run offline component checks.

ROM images, private device captures and development signing keys stay local.
Building requires the supplied firmware inputs and a prepared native build
environment, as described in the port documentation.

## Credits and licensing

Based on [tsryzenstech/hmos2sweet](https://github.com/tsryzenstech/hmos2sweet),
with OP13-specific adaptations. Component origins and licenses are listed in
[NOTICE.md](NOTICE.md); see also [LICENSE](LICENSE).
