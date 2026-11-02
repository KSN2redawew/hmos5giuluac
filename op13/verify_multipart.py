#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Verify packed OP13 partition contents, permissions and unchanged input bytes."""
import argparse
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import zipfile
from build_multipart import digest


def remove_readback(target, output):
    assert target.parent == output and not target.is_symlink()
    # Extracted filesystem directories can be read-only. Change only the
    # generated directory permissions; never follow their symlinks.
    for current, directories, files in os.walk(target, followlinks=False):
        os.chmod(current, Path(current).stat().st_mode | 0o700)
    shutil.rmtree(target)


def fs_bytes(path):
    with path.open('rb') as file:
        file.seek(1024)
        header = file.read(128)
    if header[:4] != b'\xe2\xe1\xf5\xe0':
        raise RuntimeError('not EROFS: ' + str(path))
    return (1 << header[12]) * struct.unpack_from('<I', header, 36)[0]


def equal_prefix(original, candidate, start, end):
    with original.open('rb') as before, candidate.open('rb') as after:
        before.seek(start)
        after.seek(start)
        for offset in range(start, end, 4 * 1024 * 1024):
            length = min(4 * 1024 * 1024, end - offset)
            a, b = before.read(length), after.read(length)
            if len(a) != length or a != b:
                raise RuntimeError('original EROFS block data changed: ' + str(candidate))


def read_metadata(workspace, output, partition):
    target = output / 'packed-metadata' / partition
    subprocess.run([str(workspace / 'bin/Linux/x86_64/extract.erofs'), '-i', str(output / (partition + '.img')),
        '-x', '--only-cfg', '-o', str(target)], stdout=subprocess.DEVNULL, check=True)
    config = target / 'config'
    owners = {row[0].removeprefix(partition + '/').rstrip('/'): row[1:]
              for row in (line.split() for line in (config / (partition + '_fs_config')).read_text().splitlines())}
    labels = {}
    for line in (config / (partition + '_file_contexts')).read_text().splitlines():
        name, label = line.split()
        literal = re.sub(r'\\(.)', r'\1', name)
        labels[literal.removeprefix('/' + partition + '/').rstrip('/')] = label
    return owners, labels


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--workspace', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--keep-extracted', action='store_true', help='retain temporary filesystem readback trees')
    args = parser.parse_args()
    root, out = args.workspace.resolve(), args.output.resolve()
    report = json.loads((out / 'manifest.json').read_text())
    assert report['status'] == 'experimental-op13-only-multipart-dsu'
    sources = {'system': 'hmos5/system.img', 'vendor': 'op13_stock/vendor.img',
        'odm': 'op13_stock/odm.img', 'sys_prod': 'hmos5/sys_prod.img',
        'chip_prod': 'hmos5/chip_prod.img', 'cust': 'hmos5/cust.img', 'patch_hw': 'hmos5/patch.img'}
    payload_checks = {}
    for name, source in sources.items():
        image = out / (name + '.img')
        assert digest(image) == report['partitions'][name]['sha256'], name
        if name in ('system', 'vendor'):
            # Incremental system builds update old directory/inode blocks too.
            # Check the logical files below instead of assuming a byte prefix.
            continue
        start = 0
        equal_prefix(root / source, image, start, fs_bytes(root / source))
        payload_checks[name] = {'original_erofs_bytes_verified': fs_bytes(root / source) - start,
                               'superblock_changed_by_incremental_build': bool(start)}
        print('PASS original filesystem payload retained:', name, flush=True)
    sys_owners, sys_labels = read_metadata(root, out, 'system')
    vendor_owners, vendor_labels = read_metadata(root, out, 'vendor')
    system_trees = []
    for name, image in [('original-system-tree', root / 'hmos5/system.img'),
                        ('verified-system-tree', out / 'system.img')]:
        target = out / name
        if target.exists():
            remove_readback(target, out)
        subprocess.run(['fsck.erofs', '--extract=' + str(target), '--no-preserve-owner', '--preserve-perms',
                        str(image)], check=True, stdout=subprocess.DEVNULL)
        system_trees.append(target)
    before, after = system_trees
    modified = {str(path.relative_to(out / 'system-patch'))
                for path in (out / 'system-patch').rglob('*') if path.is_file()}
    system_files_checked = 0
    for path in before.rglob('*'):
        name = str(path.relative_to(before))
        if name in modified:
            continue
        candidate = after / name
        if path.is_symlink():
            assert candidate.is_symlink() and path.readlink() == candidate.readlink(), name
            system_files_checked += 1
        elif path.is_file():
            assert candidate.is_file() and not candidate.is_symlink() and digest(path) == digest(candidate), name
            system_files_checked += 1
    assert digest(before / 'system/bin/init') == digest(after / 'system/bin/hmos-native-init')
    print('PASS original system files/symlinks outside explicit patch paths:', system_files_checked, flush=True)
    base = json.loads((out / 'vendor-base-content.json').read_text())
    unpacked = out / 'verified-vendor-tree'
    if unpacked.exists():
        remove_readback(unpacked, out)
    subprocess.run(['fsck.erofs', '--extract=' + str(unpacked), '--no-preserve-owner', '--preserve-perms',
                    str(out / 'vendor.img')], check=True, stdout=subprocess.DEVNULL)
    for name, record in base.items():
        path = unpacked / name
        if 'symlink' in record:
            assert path.is_symlink() and str(path.readlink()) == record['symlink'], name
        else:
            assert path.is_file() and not path.is_symlink() and digest(path) == record['sha256'], name
    expected = {'system/bin/init': ('0755', 'init_exec'),
        'system/bin/hmos-native-init': ('0755', 'init_exec'),
        'system/bin/storage_daemon': ('0755', 'storage_daemon_exec'),
        'system/bin/hmos-oobedone': ('0755', 'system_bin_file'),
        'system/lib64/libop13_local_storage.so': ('0644', 'system_lib_file'),
        'hmos/startup/init.cfg': ('0644', 'system_etc_file'),
        'hmos/startup/hdf_default.hcb': ('0644', 'chip_prod_file')}
    for path, (mode, label) in expected.items():
        if sys_owners[path][2] != mode or sys_labels[path] != 'u:object_r:' + label + ':s0':
            raise RuntimeError('incorrect packed system mode/label: ' + path)
    for name in ('dev', 'proc', 'sys', 'mnt', 'metadata', 'debug_ramdisk', 'second_stage_resources',
                 'first_stage_ramdisk', 'vendor', 'odm', 'sys_prod', 'chip_prod', 'cust', 'patch_hw'):
        subprocess.run(['dump.erofs', '--path=/' + name, str(out / 'system.img')],
            check=True, stdout=subprocess.DEVNULL)
    original_owners = {}
    for line in (root / 'out/vendor-layout/stock/config/vendor_fs_config').read_text().splitlines():
        row = line.split()
        original_owners[row[0].removeprefix('vendor/').rstrip('/')] = row[1:]
    for name, owner in original_owners.items():
        if name in ('/', ''):
            continue
        if vendor_owners.get(name) != owner:
            raise RuntimeError('stock vendor ownership/mode changed: ' + name)
    original_contexts = []
    for line in (root / 'out/vendor-layout/donor/config/vendor_file_contexts').read_text().splitlines():
        pattern, label = line.split()
        original_contexts.append((re.compile(pattern), label))
    for record in report['vendor_additions']['files']:
        name = record['path']
        assert not record['overrides_op13_path']
        assert vendor_owners[name] == record['metadata'], name
        label = next(value for pattern, value in original_contexts if pattern.fullmatch('/vendor/' + name))
        assert vendor_labels[name] == label, name
    for name, label in [('etc', 'vendor_etc_file'), ('etc/selinux', 'vendor_etc_file'),
                        ('bin', 'vendor_bin_file'), ('lib64', 'vendor_lib_file')]:
        assert vendor_labels[name] == 'u:object_r:' + label + ':s0', name
    package = out / report['zip']['file']
    assert digest(package) == report['zip']['sha256']
    with zipfile.ZipFile(package) as archive:
        assert set(archive.namelist()) == {name + '.img' for name in sources}
        assert not any(name in archive.namelist() for name in ('boot.img', 'vendor_boot.img', 'dtbo.img', 'vbmeta.img'))
    originals = json.loads((root / 'out/stock-dsu-audit/report.json').read_text())['images']
    for name, entry in originals.items():
        assert digest(root / name) == entry['sha256'], name
    result = {'passed': True, 'original_input_images_unchanged': len(originals),
        'partition_payload_checks': payload_checks, 'system_mode_label_checks': len(expected),
        'stock_vendor_metadata_entries_checked': len(original_owners),
        'native_vendor_addition_checks': len(report['vendor_additions']['files']),
        'op13_vendor_file_contents_or_symlinks_preserved': len(base),
        'unmodified_system_file_contents_or_symlinks_preserved': system_files_checked,
        'zip_exactly_seven_partition_images': True,
        'scope': 'Offline block retention, file metadata, hashes and archive membership; no device boot or ARB acceptance test'}
    (out / 'packed-validation.json').write_text(json.dumps(result, indent=2) + '\n')
    if not args.keep_extracted:
        for target in [*system_trees, unpacked]:
            remove_readback(target, out)
    print('PASS packed metadata, OP13 base preservation, ZIP membership and all original input hashes')


if __name__ == '__main__':
    main()
