# Input path checked in the supplied donor

The input investigation found two different native interfaces. Their presence
does not imply both are used by the same client:

| Library | Observed path |
| --- | --- |
| `libhdi_input.z.so` | Legacy HAL that imports `HdfIoServiceBind` and HDF event-listener functions. |
| `libinput_proxy_1.0.z.so` | Its `Get(true)` path calls `LoadHdiImpl`; the inspected `Get(false)` path returns null. |
| `libinput_interfaces_service_1.0.z.so` | Directly depends on `libhdi_input_udriver.z.so`. |
| `libhdi_input_udriver.z.so` | Existing userspace implementation that enumerates Linux `/dev/input` devices. |
| `libmmi-server.z.so` | Links the Linux `libinput-third-mmi.z.so` implementation. |

These observations come from the actual donor ELF dynamic dependencies,
exported symbols and disassembly. The local OpenHarmony input interface build
also specifies passthrough mode. They do not establish which optional clients
execute every interface on a running phone.

The donor HDF configuration separately names `libinput_driver.z.so`, which is
absent from the supplied images. The curated startup omits that stale
`input_user_host` entry. It retains the existing Linux input libraries and
the passthrough service; it adds no fake HDF driver or successful-IOCTL stub.

`check_input_backend.sh` compiles an ARM64 test against the real donor udriver
and runs it with QEMU userspace emulation inside a fresh bubblewrap namespace.
The namespace has an empty `/dev/input` and no HDF or Binder devices. The test
checks actual interface initialization, its three interface tables, real
errors for empty-device/invalid-device requests, and release of the interface.
The test does not access host input devices or run on the OP13 kernel.

Physical input events, touch-driver loading, SELinux permissions, hotplug and
delivery to SceneBoard remain unverified. This result is specific to input
backend initialization; it says nothing about the kernel features needed by
the rest of the HMOS5 framework.
