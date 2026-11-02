# hmos5dodge

Experimental **HarmonyOS NEXT 5 port for OnePlus 13**, using the stock kernel
and separate DSU partition images. Forked from
[tsryzenstech/hmos2sweet](https://github.com/tsryzenstech/hmos2sweet).

The OP13 implementation, build notes and manual test instructions are in
[op13/README.md](op13/README.md). The port uses an OP13 vendor base with selected
native HarmonyOS runtime additions and the stock ODM filesystem. No kernel,
bootloader or physical firmware image is replaced by its DSU package.

**Status: offline candidate validated; OP13 boot and hardware operation remain
unverified.** Source is included here; ROM images, device captures and signing
keys remain local.

The documentation below describes the original **Redmi Note 10 Pro** project.
Its working-feature table describes upstream's Redmi results, not OP13 results.

---

## Upstream project: hmos2sweet

**HarmonyOS 5 (OpenHarmony) for the Redmi Note 10 Pro (sweet).**

`hmos2sweet` takes a **HarmonyOS full OTA package** (`update_full_base.zip`) and turns it into a ROM you can flash
on the Redmi Note 10 Pro. The userspace stays Huawei's. What changes is underneath it:

- a LineageOS 4.14 (sm6150) kernel with the patches OpenHarmony needs;
- a set of **selectable patches** that replace the Kirin drivers with Snapdragon 732G equivalents: display, GPU,
  audio, coprocessors, telephony and more.

> Experimental project. No Huawei or Xiaomi image is included in this repository: the script takes them from
> the packages you provide. Use the resulting ROM for personal use only.

## Status

| Feature | Status |
|---|---|
| Boot, boot animation, SceneBoard, launcher, setup wizard | ✅ |
| Display 1080x2400 at 120 Hz (DRM/KMS), brightness, screen off/on | ✅ |
| GPU Adreno 618 (Mesa freedreno, OpenGL ES) | ✅ |
| Touch, volume/power keys | ✅ |
| WiFi, Bluetooth, GPS, audio | ✅ (reported working on the device) |
| Lock screen, PIN/password | 🟡 OpenHarmony's REE user_auth/pin_auth without TEE (fixed in iter42: vtable mismatch with HarmonyOS's HDI) |
| Sensors for apps (accelerometer, gyroscope, compass, light, gravity/rotation) | 🟡 HAL on the Snapdragon Sensor Core over QRTR/QMI |
| Battery, charging, off-mode charging | 🟡 standard HarmonyOS HAL on sweet's power_supply; with a charger plugged in "power off" keeps the phone off and charging |
| Vibration | 🟡 aw8624 exposed as /sys/class/leds/vibrator (timed vibration; no haptic effects) |
| Telephony: SIM, network, calls, SMS, mobile data | 🟡 our own RIL library on the Qualcomm modem over QMI/QRTR (untested without a SIM) |
| Call forwarding, call waiting, call barring, CLIP/CLIR, USSD | 🟡 QMI VOICE supplementary services (untested) |
| VoLTE / VoLTE-only calls | 🟡 attempt: IMS settings, IMS PDN, PS-only voice domain (untested) |
| NFC | 🟡 NXP PN553 driven by Huawei's NXP HAL + /sys/nfc from the kernel (untested) |
| IR blaster | 🟡 IR LED through ir-spi (/dev/lirc0) |
| Thermal | 🟡 real sweet sensors, own policy |
| USB (hdc, file transfer) | 🟡 configfs gadget for the Snapdragon controller |
| Camera | 🧪 first test build (patch 47): sweet's Android camera HAL (CamX) in an isolated Android environment + an OpenHarmony camera host |
| Fingerprint, proximity sensor | 🔴 not yet (need Qualcomm's Android/TEE blobs) |

Tested with: Huawei Pura 70 HarmonyOS 5.1 full OTA (`ADY-LGRP3-CHN 205.1.0.128(SP5)`, `update_full_base.zip`).
Other HarmonyOS 5.x versions of the Pura 70 should convert as well: the patches that edit system files check what
they find and skip the step if the file is not the expected one.

## Requirements

- **Linux** (or **WSL** on Windows) with Python 3.8+, `e2fsprogs` (`debugfs`), `unzip`, `curl`.
  `pip install brotli`, or the `brotli` command, if the sweet ROM uses `vendor.new.dat.br`.
- About **25 GB free**: extracted images, temporary trees and output.
- A **HarmonyOS full OTA** of the Huawei Pura 70 (`update_full_base.zip`, the one with `update.bin` inside).
  `fetchfw` (below) can find and download one for you.
- **A ROM for the Redmi Note 10 Pro** with block images (`vendor.new.dat.br` + `vendor.transfer.list`, e.g.
  LineageOS), or an extracted `vendor.img`. It provides sweet's GPU, IPA, audio amplifier, WiFi and camera
  firmware and its audio mixer paths.
- A phone with an **unlocked bootloader**, `fastboot` and `adb` (platform-tools) in PATH, and a recovery
  (TWRP/OrangeFox) to read the logs.

## Getting the firmware: fetchfw

`fetchfw` looks for every HarmonyOS package available for the **Huawei Pura 70**, shows the list, downloads
the one you pick (resumable, sha256-checked) and then starts `hmos2sweet.py` on it:

```sh
python3 fetchfw.py                                        # search, choose, download, port
python3 fetchfw.py -- --sweet-rom ~/lineage-sweet.zip --patches defaults
python3 fetchfw.py --list                                 # only list
python3 fetchfw.py --url <package URL>                    # check and add a package you found
python3 fetchfw.py --offline                              # only the catalogs on this computer
```

On Windows `fetchfw.bat` does the same. Where the packages come from:

- the shared catalog of this repository (`lib/fetchfw_catalog.json`, also read online from GitHub, so new
  entries reach every copy of the tool);
- public pages that list Huawei full packages (their `update_full_base.zip` links are collected);
- Huawei's update services (the HarmonyOS 5 `hota/version-query` one and the older `Check.action`): they
  normally answer only requests signed by a real phone, so they rarely add anything;
- your own list (`~/.hmos2sweet_fw.json`), filled automatically with every package found or added with `--url`.

Huawei's CDN paths contain a random id, so the packages cannot be enumerated directly. **Every candidate is
verified before it is offered**: its `filelist.xml` and the headers inside the zip (`packageinfo.mbn`,
`VERSION.mbn`, read with HTTP range requests) must describe a full BASE package of a Pura 70 (`ADY-*`);
packages of other phones are skipped. To share a new Pura 70 package with everybody, add it to
`lib/fetchfw_catalog.json` with a pull request.

## Usage

```sh
git clone https://github.com/tsryzenstech/hmos2sweet && cd hmos2sweet
tools/fetch_tools.sh                     # downloads the EROFS tools into tools/bin
python3 hmos2sweet.py --ota ~/update_full_base.zip --sweet-rom ~/lineage-sweet.zip --out out
```

The patch list opens:

```
  Patches to apply  (number = toggle, a = all, n = none, d = defaults,
                     i<number> = details, Enter = continue, q = quit)

     [=] Core: kernel, preinit, fstab
   1 [x] Display: DRM/KMS composer + buffer allocator
   2 [x] GPU: Mesa freedreno (Adreno 618) for OpenGL ES
   ...
```

- a number (or several, separated by spaces) toggles a patch;
- `a` enables everything, `n` disables everything, `d` goes back to the defaults;
- `i3` shows the description of patch 3;
- Enter continues.

Dependencies are handled automatically: enabling audio also enables "Modem/ADSP bring-up", disabling the
latter also disables audio and WiFi.

Without the menu (scripts, CI):

```sh
python3 hmos2sweet.py --ota OTA.zip --sweet-rom ROM.zip --patches defaults            # defaults
python3 hmos2sweet.py --ota OTA.zip --sweet-rom ROM.zip --patches all,-60             # all but debug-tools
python3 hmos2sweet.py --ota OTA.zip --sweet-rom ROM.zip --patches defaults --no-setup-wizard
python3 hmos2sweet.py --list                                                          # patch list
```

### Setup wizard

HarmonyOS's first-boot setup needs a WiFi connection. WiFi works on sweet, so the wizard is kept by default.
If you cannot get through it, enable the **"Remove the setup wizard"** patch (or pass `--no-setup-wizard`):

- it removes `HwStartupGuide` from `system.img` (the original files are saved in `out/removed/`);
- it marks the first boot as completed in the settings database (`device_provisioned`, `user_setup_complete`).

## Installation

The output folder contains:

| File | |
|---|---|
| `system.img`, `vendor.img`, `chip_prod.img`, `sys_prod.img`, `cust.img`, `patch.img` | logical partitions (in `super`) |
| `boot.img`, `dtbo.img` | kernel + HarmonyOS ramdisk with `hmos-preinit` |
| `install_clean.bat` / `.sh` | **first install**: wipes Android and ALL data, creates the HarmonyOS partitions |
| `update_keep_data.bat` / `.sh` | update from a previous hmos2sweet build, **data is kept** |
| `get_logs.bat` / `.sh` | from recovery: copies the kernel log, hilog and boot screenshots |
| `BUILD_INFO.txt` | OTA version and patches used |
| `removed/` | files taken out of the images (setup wizard) |

1. Phone in fastboot (Power + Volume down).
2. `install_clean.bat` (Windows) or `./install_clean.sh` (Linux). It asks you to type `YES`.
3. The first boot takes a few minutes.

To go back to Android, flash a sweet ROM again from recovery: the ROM recreates `super`.

### Power off and charging

With a charger or a PC connected, "power off" makes the PMIC start the phone again in charging mode
(`androidboot.mode=charger`). `hmos-preinit` recognises it and does not start HarmonyOS: the screen stays off
and the phone keeps charging. Unplugging the cable powers it off; holding the power key for 1.5 s boots
HarmonyOS.

### Logs

The ramdisk contains `hmos-preinit`, which saves to the `cache` partition on every boot:

- the kernel log;
- snapshots of hilog, processes, threads and services;
- a screenshot.

To read them: boot the recovery (Power + Volume up), then run `get_logs.bat` / `get_logs.sh`. See
[docs/TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md).

## Repository layout

```
hmos2sweet.py          converter (patch menu, OTA extraction, image build, flash scripts)
fetchfw.py             finds/downloads Pura 70 firmware and runs hmos2sweet.py (lib/fetchfw.py)
lib/                   Python modules: Huawei OTA, EROFS, boot.img, sweet ROM, patches, scripts, fetchfw
patches/NN-name/       one folder per patch: patch.json (+ hook.py) + files/ with the binaries
prebuilt/              kernel (Image.gz, dtb, dtbo.img) and hmos-preinit
src/                   sources of everything in patches/ and prebuilt/
  kernel/patches/        patches on LineageOS android_kernel_xiaomi_sm6150 (lineage-24.0)
  preinit/               hmos-preinit (/init: maps super, logs, charger mode, then HarmonyOS /bin/init_early)
  display/               DRM/KMS composer + dma-buf allocator (OpenHarmony VDI)
  mesa/                  Mesa 26.1 patches: OHOS EGL platform, YUV import
  audio/                 audio VDI HAL on tinyalsa
  userauth/ huks/ iam/   user_auth stub without TEE, software HUKS, OpenHarmony REE user_auth/pin_auth
  remoteproc/            sweet-rproc + patches to rmtfs/tqftpserv/pd-mapper (linux-msm)
  sensors/               hexagonrpcd for the ADSP + sensors HAL (SSC over QRTR/QMI)
  bluetooth/             Bluetooth vendor library for WCN3990
  ril/ gnss/ ir/         RIL over QMI (calls, SMS, data, supplementary services, USSD, IMS), GNSS over QMI LOC, IR
  camera/                camera: Android HAL bridge (cambridge), namespace launcher, OpenHarmony camera host (WIP)
  oobe/ idle/ stubs/     sweet-oobedone (skip setup), placeholders for Kirin daemons, link stubs
build/                 scripts to rebuild the kernel and the binaries (see docs/BUILDING.md)
tools/                 fetch_tools.sh (erofs-utils), gen_patch_docs.py
flash/                 log scripts
docs/                  architecture, patches, building, known issues
```

Every patch is described in [docs/PATCHES.md](docs/PATCHES.md). How the port works is explained in
[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md); rebuilding the binaries in [docs/BUILDING.md](docs/BUILDING.md).

## Licenses

The scripts in this repository are Apache-2.0. The individual parts have different licenses:

- kernel and kernel patches, hmos-preinit: GPL-2.0;
- code derived from OpenHarmony: Apache-2.0;
- Mesa: MIT;
- tinyalsa and linux-msm: BSD-3-Clause;
- sqlite: public domain.

Details are in [NOTICE.md](NOTICE.md). HarmonyOS, Huawei, Xiaomi and Redmi are trademarks of their respective
owners. This project is not affiliated with any of them.
