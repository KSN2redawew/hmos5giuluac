#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Build and exercise the offline DSU components without connecting a phone."""
import hashlib
import json
import os
from pathlib import Path
import subprocess

REPO = Path(__file__).resolve().parents[1]
WORKSPACE = REPO.parent
OUTPUT = WORKSPACE / 'out/component-validation'
SYSROOT = REPO / 'build/work/op13-sysroot'


def run(name, command, env=None, expected=0):
    result = subprocess.run([str(item) for item in command], cwd=REPO, env=env,
                            text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=60)
    (OUTPUT / (name + '.log')).write_text(result.stdout)
    if result.returncode != expected:
        raise RuntimeError(name + ': ' + result.stdout[-4000:])
    print('PASS', name, flush=True)
    return {'name': name, 'exit_status': result.returncode, 'passed': True}


def main():
    OUTPUT.mkdir(parents=True, exist_ok=True)
    bootstrap = WORKSPACE / 'out/op13-bootstrap'
    storage = WORKSPACE / 'out/op13-storage'
    display = WORKSPACE / 'out/op13-display'
    qemu = WORKSPACE / 'work/qemu-aarch64-static'
    ndk = Path(os.environ.get('ANDROID_NDK_HOME', '/opt/android-ndk'))
    cc = ndk / 'toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android30-clang'
    tests = []
    tests.append(run('bootstrap-build', [cc, '-static', '-O2', '-Wall', '-Wextra', '-Werror',
        'op13/dsu_bootstrap.c', '-o', bootstrap / 'init']))
    tests.append(run('guard-test-build', ['clang', '-O1', '-g', '-fsanitize=address,undefined',
        '-Wall', '-Wextra', '-Werror', 'op13/bootstrap_test.c', '-o', bootstrap / 'bootstrap-test']))
    tests.append(run('device-filter-and-data-scan', [bootstrap / 'bootstrap-test']))
    tests.append(run('bootstrap-refuses-non-handoff', [qemu, bootstrap / 'init', '--not-pid1'], expected=2))
    environment = dict(os.environ, SYSROOT=str(SYSROOT))
    tests.append(run('storage-library-build', ['bash', 'build/ohos-clang', '-O2', '-fPIC', '-Wall', '-Wextra', '-Werror',
        '-shared', 'op13/local_storage.c', '-Wl,-soname,libop13_local_storage.so', '-Wl,--no-undefined',
        '-ldl', '-lc', '-o', storage / 'libop13_local_storage.so'], env=environment))
    tests.append(run('storage-test-build', ['bash', 'build/ohos-clang', '-O2', '-Wall', '-Wextra', '-Werror',
        'op13/local_storage_test.c', '-L' + str(storage), '-lop13_local_storage', '-lc',
        '-o', storage / 'storage-test'], env=environment))
    (storage / 'namespace-marker').touch()
    sandbox = ['bwrap', '--die-with-parent', '--unshare-all', '--uid', '0', '--gid', '0',
               '--cap-add', 'CAP_SYS_ADMIN', '--tmpfs', '/', '--proc', '/proc', '--dev', '/dev',
               '--tmpfs', '/tmp', '--dir', '/etc', '--dir', '/mnt', '--dir', '/data',
               '--ro-bind', storage / 'namespace-marker', '/data/hmos-storage-test-namespace',
               '--ro-bind', storage, '/test', '--ro-bind', SYSROOT, '/runtime',
               '--ro-bind', qemu, '/qemu', '/qemu', '-L', '/runtime', '-E',
               'LD_LIBRARY_PATH=/test:/runtime/usr/lib', '/test/storage-test']
    tests.append(run('arm64-local-storage-mounts', sandbox))
    tests.append(run('display-vtable-abi', ['python3', 'op13/check_display_abi.py',
        WORKSPACE / 'work/inspect/vendor-lib64/libdisplay_composer_vdi_impl.z.so',
        display / 'libdisplay_composer_vdi_impl.z.so', '--output', display / 'abi.json']))
    tests.append(run('arm64-display-mode-behavior', ['bwrap', '--die-with-parent', '--unshare-all',
        '--ro-bind', '/', '/', '--dev', '/dev', '--proc', '/proc', '--tmpfs', '/tmp', qemu,
        '-L', SYSROOT, '-E', 'LD_LIBRARY_PATH=' + str(display) + ':' + str(SYSROOT / 'usr/lib'),
        display / 'display-mode-test']))
    for prefix in ('', 'developer-'):
        policy = WORKSPACE / 'out/op13-policy'
        tests.append(run(prefix + 'policy-delta', ['python3', 'op13/validate_policy_delta.py',
            policy / (prefix + 'before.cil'), policy / (prefix + 'after.cil'),
            '--output', policy / (prefix + 'validation.json')]))
    image = bootstrap / 'test-data.img'
    with image.open('wb') as file:
        file.truncate(64 * 1024 * 1024)
    tests.append(run('arm64-ext4-initialization', ['bwrap', '--die-with-parent', '--unshare-all',
        '--uid', '0', '--gid', '0', '--tmpfs', '/', '--proc', '/proc', '--dev', '/dev',
        '--tmpfs', '/tmp', '--dir', '/etc', '--ro-bind', WORKSPACE / 'work/inspect/system/system/bin/mke2fs', '/mke2fs',
        '--bind', bootstrap, '/test', '--ro-bind', SYSROOT, '/runtime', '--ro-bind', qemu, '/qemu',
        '/qemu', '-L', '/runtime', '-E', 'LD_LIBRARY_PATH=/runtime/usr/lib', '/mke2fs',
        '-t', 'ext4', '-b', '4096', '-I', '256', '-O', 'encrypt,verity', '-F', '-m', '0',
        '-L', 'hmos-dsu', '/test/test-data.img']))
    tests.append(run('formatted-ext4-consistency', ['e2fsck', '-fn', image]))
    tests.append(run('startup-audit-regressions', ['python3', '-m', 'unittest', 'discover', '-s', 'op13', '-p', 'test_*.py']))
    image.unlink()
    artifacts = [bootstrap / 'init', storage / 'libop13_local_storage.so',
        display / 'libdisplay_buffer_vdi_impl.z.so', display / 'libdisplay_composer_vdi_impl.z.so',
        WORKSPACE / 'out/op13-policy/policy.31', WORKSPACE / 'out/op13-policy/developer_policy']
    record = {'passed': True, 'tests': tests,
              'artifact_sha256': {str(p.relative_to(WORKSPACE)): hashlib.sha256(p.read_bytes()).hexdigest() for p in artifacts},
              'scope': 'Offline userspace/logic/format tests. No OP13 kernel boot, BPF attachment, native framework boot or hardware rendering tested.'}
    (OUTPUT / 'validation.json').write_text(json.dumps(record, indent=2) + '\n')
    print('All offline component checks passed; hardware/DSU boot remains untested.')


if __name__ == '__main__':
    main()
