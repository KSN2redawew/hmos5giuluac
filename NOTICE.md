# Components and licenses

| Path | Origin | License |
|---|---|---|
| `hmos2sweet.py`, `fetchfw.py`, `lib/`, `patches/*/patch.json`, `patches/*/hook.py`, `tools/`, `build/*.sh`, `flash/` | this project | Apache-2.0 |
| `src/kernel/patches`, `prebuilt/kernel` | LineageOS android_kernel_xiaomi_sm6150 + this project's patches (parts from OpenHarmony kernel_linux_5.10) | GPL-2.0 |
| `src/preinit`, `prebuilt/ramdisk/init`, `prebuilt/tools/lpwrite` | this project | GPL-2.0 |
| `src/display`, `patches/10-display/files` | derived from OpenHarmony drivers_peripheral (display) | Apache-2.0 |
| `src/display/hdi-gen` | headers generated from OpenHarmony HDI IDLs | Apache-2.0 |
| `op13/display` | OP13 adaptations of the upstream OpenHarmony-derived display implementation and generated HDI headers | Apache-2.0 |
| `op13` (except the files listed separately below) | OP13 port tooling, DSU handoff, storage integration and offline checks | Apache-2.0 |
| `op13/stock_probe.c`, `op13/build_probe.sh` | read-only stock-kernel diagnostic | MIT |
| `src/audio/vdi` | OpenHarmony drivers_peripheral audio VDI headers | Apache-2.0 |
| `src/audio/sweet_audio_vdi.cpp`, `patches/31-audio/files` | this project, with tinyalsa (BSD-3-Clause) | Apache-2.0 |
| `src/huks`, `patches/21-huks-software/files` | OpenHarmony security_huks, OpenSSL 3 | Apache-2.0 |
| `src/iam`, `patches/46-pin-lock/files` | OpenHarmony drivers_peripheral user_auth / pin_auth (OpenHarmony-5.1.0-Release) | Apache-2.0 |
| `src/userauth`, `src/oobe`, `src/idle`, `src/remoteproc/sweet-rproc.c` | this project | Apache-2.0 |
| `src/ril`, `src/gnss`, `src/ir`, `patches/42-ril`, `patches/43-consumer-ir`, `patches/44-gnss` | this project (interfaces from OpenHarmony telephony_ril_adapter / drivers_peripheral) | Apache-2.0 |
| `src/camera` | this project (headers from AOSP bionic/libhardware/system_media: Apache-2.0) | Apache-2.0 |
| `src/remoteproc/patches`, `patches/30-remoteproc/files` | linux-msm qrtr, rmtfs, tqftpserv, pd-mapper | BSD-3-Clause |
| `src/mesa`, `patches/11-gpu-mesa/files` | Mesa 3D | MIT |
| `src/sensors/compat`, `src/sensors/sweet_fastrpc.c`, `patches/34-sensors-pd/files` | linux-msm hexagonrpc + shim for the downstream adsprpc driver | GPL-3.0 |
| `src/sensors/sweet_sensors_vdi.cpp`, `patches/39-sensors-hal/files` | this project | Apache-2.0 |
| `src/sensors/vdi` | OpenHarmony drivers_peripheral sensors VDI headers | Apache-2.0 |
| `src/bluetooth`, `patches/35-bluetooth/files` | this project (WCN3990 sequence as in Linux btqca/hci_qca) | Apache-2.0 |
| `patches/50-remove-setup-wizard/files`, `patches/60-debug-tools/files` | sqlite | public domain |
| `build/sysroot-base.tar.xz` | musl (MIT), LLVM libc++ (Apache-2.0 with LLVM exception), Linux UAPI (GPL-2.0 WITH Linux-syscall-note), libdrm (MIT) | various |
| `tools/bin` (downloaded) | sekaiacg/erofs-utils | GPL-2.0 / Apache-2.0 |

No Huawei or Xiaomi image or library is included: `hmos2sweet.py` reads them from the packages the user
provides at conversion time, and `fetchfw.py` only downloads from Huawei's public update CDN on the user's
request.
