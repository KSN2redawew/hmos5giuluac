#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Select OHOS runtime additions for an OP13-based vendor, without donor firmware."""
import argparse
from collections import defaultdict, deque
import hashlib
import json
import mmap
from pathlib import Path
import re
import shutil
import struct
import subprocess


def elf_info(path):
    """Read ELF64 dynamic dependencies and literal library names, without execution."""
    with path.open('rb') as file:
        if file.read(6) != b'\x7fELF\x02\x01':
            return None
        with mmap.mmap(file.fileno(), 0, access=mmap.ACCESS_READ) as data:
            if struct.unpack_from('<H', data, 18)[0] != 183:
                return None
            offset = struct.unpack_from('<Q', data, 32)[0]
            size, count = struct.unpack_from('<HH', data, 54)
            headers = [struct.unpack_from('<IIQQQQQQ', data, offset + i * size) for i in range(count)]
            dynamic = next((p for p in headers if p[0] == 2), None)
            entries = [] if dynamic is None else [struct.unpack_from('<qQ', data, off)
                for off in range(dynamic[2], dynamic[2] + dynamic[5], 16)]
            address = next((value for key, value in entries if key == 5), None)
            string_offset = None
            if address is not None:
                segment = next(p for p in headers if p[0] == 1 and p[3] <= address < p[3] + p[5])
                string_offset = segment[2] + address - segment[3]
            needed = []
            for key, value in entries:
                if key == 1:
                    start = string_offset + value
                    needed.append(data[start:data.find(b'\0', start)].decode())
            literals = {x.group().decode() for x in re.finditer(rb'[A-Za-z0-9_+.-]+\.so(?:\.[0-9]+)*', data)}
            return {'needed': needed, 'literal_libraries': sorted(literals)}


def metadata(path):
    result = {}
    for line in path.read_text().splitlines():
        parts = line.split()
        if parts[0].startswith('vendor/'):
            result[parts[0][7:].rstrip('/')] = parts[1:4]
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--workspace', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    workspace, output = args.workspace.resolve(), args.output.resolve()
    if not output.is_relative_to(workspace / 'out') or (output.exists() and any(output.iterdir())):
        parser.error('use a new empty workspace/out directory')
    tree = output / 'tree'
    tree.mkdir(parents=True)
    donor_meta = metadata(workspace / 'out/vendor-layout/donor/config/vendor_fs_config')
    stock_meta = metadata(workspace / 'out/vendor-layout/stock/config/vendor_fs_config')
    donor_libs = workspace / 'work/inspect/vendor-lib64'
    system = workspace / 'work/inspect/system/system'
    libraries = {str(p.relative_to(donor_libs)): p for p in donor_libs.rglob('*') if p.is_file()}
    by_name = defaultdict(list)
    for name in libraries:
        by_name[Path(name).name].append('lib64/' + name)
    system_names = {p.name for p in (system / 'lib64').rglob('*') if p.is_file() or p.is_symlink()}
    # Keep OP13's hardware OpenCL implementation at its original path. The
    # donor's HiSilicon GPU stack is not an OP13 adaptation; initial rendering
    # uses software Mesa. Native OpenCL acceleration remains unsupported.
    excluded_hardware_libraries = {'libOpenCL.so', 'libhvgr_v200.so', 'vulkan.hvgr_v200.so'}
    reasons = defaultdict(set)
    binaries = set()
    for config in (workspace / 'out/op13-startup').rglob('*.cfg'):
        for service in json.loads(config.read_text()).get('services', []):
            path = service.get('path', [''])[0]
            if path.startswith('/vendor/bin/'):
                binaries.add(path.removeprefix('/vendor/'))
    binaries.add('bin/hdf_devhost')
    for path in binaries:
        destination = tree / path
        destination.parent.mkdir(parents=True, exist_ok=True)
        subprocess.run(['fsck.erofs', '--extract=' + str(destination), '--path=/' + path,
            '--no-preserve-owner', '--preserve-perms', str(workspace / 'hmos5/vendor.img')], check=True,
            stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        reasons[path].add('executable in selected native startup graph')
    # These HDF modules are dynamically loaded, rather than DT_NEEDED entries.
    hcs = (workspace / 'out/op13-startup/hdf-op13.hcs').read_text()
    roots = set(re.findall(r'moduleName\s*=\s*"([^"]+)"', hcs))
    roots |= {'libdisplay_buffer_vdi_impl.z.so', 'libdisplay_composer_vdi_impl.z.so',
              'libEGL_impl.so', 'libGLESv2_impl.so', 'libhdi_input.z.so', 'libhdi_input_udriver.z.so'}
    unavailable_roots = []
    for name in roots:
        if name not in by_name:
            if name in system_names:
                continue
            unavailable_roots.append(name)
            continue
        for path in by_name[name]:
            reasons[path].add('selected HDF/display runtime root')
    print('Finding vendor library references in donor system ELFs', flush=True)
    for directory in (system / 'lib64', system / 'bin'):
        for path in directory.rglob('*'):
            if not path.is_file() or path.is_symlink():
                continue
            info = elf_info(path)
            if info is None:
                continue
            for name in set(info['needed']) | set(info['literal_libraries']):
                if name in excluded_hardware_libraries:
                    continue
                for candidate in by_name.get(name, []):
                    reasons[candidate].add('referenced by /system/' + str(path.relative_to(system)))
    queue = deque(reasons)
    scanned = set()
    unresolved = defaultdict(set)
    while queue:
        name = queue.popleft()
        if name in scanned:
            continue
        scanned.add(name)
        source = tree / name if name.startswith('bin/') else donor_libs / name.removeprefix('lib64/')
        info = elf_info(source)
        if not info:
            raise RuntimeError('selected runtime is not AArch64 ELF: ' + name)
        # Runtime library literals cover dlopen plugins; their presence in a
        # string is an inclusion reason, not proof the path executes at boot.
        for dep in set(info['needed']) | set(info['literal_libraries']):
            if dep in excluded_hardware_libraries:
                if dep in info['needed']:
                    unresolved[dep].add(name)
                continue
            if dep in system_names:
                continue
            candidates = by_name.get(dep, [])
            if not candidates and dep in info['needed']:
                unresolved[dep].add(name)
            for candidate in candidates:
                reasons[candidate].add('dependency/reference of ' + name)
                if candidate not in scanned:
                    queue.append(candidate)
    configs = {'etc/native_packages.xml'}
    for name in donor_meta:
        if name.startswith(('etc/selinux/', 'etc/hdfconfig/', 'etc/passthrough/')):
            configs.add(name)
    for name in sorted(configs):
        source = workspace / 'work/inspect/vendor' / name
        if source.is_file():
            destination = tree / name
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source, destination)
            reasons[name].add('native policy/HDF/passthrough configuration')
    for name in sorted(reasons):
        if name.startswith('lib64/'):
            destination = tree / name
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(donor_libs / name[6:], destination)
    for name in ('libdisplay_buffer_vdi_impl.z.so', 'libdisplay_composer_vdi_impl.z.so'):
        shutil.copyfile(workspace / 'out/op13-display' / name, tree / 'lib64' / name)
        reasons['lib64/' + name].add('rebuilt OP13 native display implementation')
    # The candidate native policy replaces the donor binary only in the copy.
    for name in ('policy.31', 'developer_policy'):
        shutil.copyfile(workspace / 'out/op13-policy' / name, tree / 'etc/selinux/prebuild_sepolicy' / name)
    records = []
    for path in sorted(tree.rglob('*')):
        relative = str(path.relative_to(tree))
        if path.is_file():
            if not relative.startswith(('bin/', 'lib64/', 'etc/')) or relative.endswith(('.ko', '.sec')):
                raise RuntimeError('hardware/firmware payload escaped selection: ' + relative)
            owner = donor_meta[relative]
            path.chmod(int(owner[2], 8))
            records.append({'path': relative, 'sha256': hashlib.sha256(path.read_bytes()).hexdigest(),
                'metadata': owner, 'reasons': sorted(reasons[relative]), 'overrides_op13_path': relative in stock_meta})
    original_contexts = workspace / 'out/vendor-layout/donor/config/vendor_file_contexts'
    shutil.copyfile(original_contexts, output / 'donor-file-contexts')
    report = {'base': 'op13_stock/vendor.img', 'whole_donor_vendor_used': False,
        'donor_firmware_or_kernel_modules_selected': False, 'files': records,
        'unresolved_needed': {name: sorted(users) for name, users in sorted(unresolved.items())},
        'unavailable_hdf_roots': sorted(unavailable_roots),
        'excluded_donor_hardware_libraries': sorted(excluded_hardware_libraries),
        'dependency_check_scope': 'ELF names/DT_NEEDED/literal references against extracted system/vendor trees; not symbol-version or namespace execution validation',
        'excluded_donor_top_levels': sorted({name.split('/')[0] for name in donor_meta} - {'bin', 'etc', 'lib64', ''})}
    (output / 'manifest.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'selected_files': len(records), 'bytes': sum((tree / r['path']).stat().st_size for r in records),
        'stock_files_overridden': sum(r['overrides_op13_path'] for r in records),
        'unresolved_needed': report['unresolved_needed']}, indent=2))


if __name__ == '__main__':
    main()
