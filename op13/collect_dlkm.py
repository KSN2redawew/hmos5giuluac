#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Read the current Android DLKM filesystems over ADB; write only PC files.

Run manually with Android booted and root available through su. The script
does not flash, mount, reboot, load modules, or write anything on the phone.
Mounted dm-verity views may omit original AVB footers; this is intentional.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shlex
import subprocess


def remote(adb, command):
    return subprocess.check_output([adb, 'exec-out', command], stderr=subprocess.PIPE, timeout=30).decode().strip()


def source_for_mount(mounts, mountpoint):
    matches = [line.split() for line in mounts.splitlines()
               if len(line.split()) >= 4 and line.split()[1] == mountpoint]
    if len(matches) != 1:
        raise ValueError('expected exactly one mounted ' + mountpoint)
    source, _, fs, options = matches[0][:4]
    if not re.fullmatch(r'/dev/block/[A-Za-z0-9_./-]+', source) or '..' in source.split('/'):
        raise ValueError('unexpected mounted block source: ' + source)
    if fs not in ('erofs', 'ext4') or 'ro' not in options.split(','):
        raise ValueError('expected a read-only EROFS/ext4 DLKM mount: ' + mountpoint)
    return source, fs


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True, help='new empty directory on the PC')
    parser.add_argument('--adb', default='adb')
    args = parser.parse_args()
    output = args.output.expanduser().resolve()
    if output.exists() and any(output.iterdir()):
        parser.error('output must be a new empty directory')
    identity = remote(args.adb, "su -c 'id'")
    if not re.search(r'\buid=0\b', identity):
        raise RuntimeError('root through su is required: ' + identity)
    sdk = remote(args.adb, 'getprop ro.build.version.sdk')
    if not sdk.isdecimal() or remote(args.adb, 'getprop ro.gsid.image_running') in ('1', 'true'):
        raise RuntimeError('run this while the regular Android installation is booted')
    mounts = remote(args.adb, 'cat /proc/mounts')
    sources = {name: source_for_mount(mounts, '/' + name) for name in ('vendor_dlkm', 'system_dlkm')}
    report = {'kernel_release': remote(args.adb, 'uname -r'),
        'android_fingerprint': remote(args.adb, 'getprop ro.build.fingerprint'),
        'android_sdk': sdk, 'android_patch': remote(args.adb, 'getprop ro.build.version.security_patch'),
        'slot_suffix': remote(args.adb, 'getprop ro.boot.slot_suffix'),
        'device_writes_performed': False, 'images': {},
        'scope': 'Read-only mounted filesystem views; original partition AVB footers may be absent'}
    output.mkdir(parents=True, exist_ok=True)
    for name, (source, fs) in sources.items():
        print('Reading', name, 'from current mounted source', source, flush=True)
        target = output / (name + '.img')
        partial = target.with_suffix('.img.part')
        command = 'su -c ' + shlex.quote('cat ' + shlex.quote(source))
        digest, size = hashlib.sha256(), 0
        process = subprocess.Popen([args.adb, 'exec-out', command], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        try:
            with partial.open('xb') as file:
                while chunk := process.stdout.read(1024 * 1024):
                    size += len(chunk)
                    if size > 2 * 1024**3:
                        raise RuntimeError('unexpected DLKM filesystem larger than 2 GiB; stopping read')
                    file.write(chunk)
                    digest.update(chunk)
            stderr = process.stderr.read().decode(errors='replace')
            if process.wait() != 0:
                raise RuntimeError('ADB read failed: ' + stderr)
            with partial.open('rb') as file:
                header = file.read(1100)
            valid = header[1024:1028] == b'\xe2\xe1\xf5\xe0' if fs == 'erofs' else header[1080:1082] == b'\x53\xef'
            if not valid:
                raise RuntimeError('dump does not contain the expected filesystem: ' + name)
            partial.rename(target)
        finally:
            if process.poll() is None:
                process.kill()
            process.wait()
            partial.unlink(missing_ok=True)
        report['images'][name] = {'file': target.name, 'source': source, 'filesystem': fs,
                                 'bytes': size, 'sha256': digest.hexdigest()}
        (output / 'device.json').write_text(json.dumps(report, indent=2) + '\n')
    (output / 'SHA256SUMS').write_text(''.join(row['sha256'] + '  ' + row['file'] + '\n'
                                            for row in report['images'].values()))
    print('Saved both DLKM filesystems and device.json to', output)


if __name__ == '__main__':
    main()
