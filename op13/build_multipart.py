#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Build the OP13-only port as separate DSU partition images and a ZIP."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import zipfile


def digest(path):
    result = hashlib.sha256()
    with path.open('rb') as file:
        for chunk in iter(lambda: file.read(4 * 1024 * 1024), b''):
            result.update(chunk)
    return result.hexdigest()


def command(*args):
    subprocess.run([str(a) for a in args], check=True)


def metadata(workspace, kind, partition):
    root = workspace / 'out/vendor-layout' / kind / 'config'
    owners = {}
    for line in (root / (partition + '_fs_config')).read_text().splitlines():
        fields = line.split()
        name = fields[0].removeprefix(partition + '/').rstrip('/')
        owners[name] = fields[1:]
    labels = []
    for line in (root / (partition + '_file_contexts')).read_text().splitlines():
        name, label = line.split()
        if name.startswith('/' + partition + '/'):
            labels.append((re.compile(name[len(partition) + 1:]), label))
    return owners, labels


def patch_image(workspace, output, partition, source, tree, owners, labels, special, full=False):
    if full:
        merged = output / (partition + '-tree')
        command('fsck.erofs', '--extract=' + str(merged), '--no-preserve-owner', '--preserve-perms', source)
        original_contents = {}
        for path in sorted(merged.rglob('*')):
            name = str(path.relative_to(merged))
            if path.is_symlink():
                original_contents[name] = {'symlink': str(path.readlink())}
            elif path.is_file():
                original_contents[name] = {'sha256': digest(path)}
        (output / (partition + '-base-content.json')).write_text(json.dumps(original_contents, indent=2) + '\n')
        shutil.copytree(tree, merged, symlinks=True, dirs_exist_ok=True)
        tree = merged
    contexts = output / (partition + '-file-contexts')
    config = output / (partition + '-fs-config')
    fs, fc = [], []
    for path in [tree] + sorted(tree.rglob('*')):
        name = str(path.relative_to(tree)) if path != tree else ''
        owner = owners.get(name, ['0', '0', '0755' if path.is_dir() else '0644'])
        label = next((value for pattern, value in labels if re.fullmatch(pattern, '/' + name)),
                     'u:object_r:rootfs:s0' if partition == 'system' else 'u:object_r:vendor_file:s0')
        for pattern, value in special:
            if re.fullmatch(pattern, '/' + name):
                label = 'u:object_r:' + value + ':s0'
        if name in ('system/bin/init', 'system/bin/hmos-native-init', 'system/bin/storage_daemon', 'system/bin/hmos-oobedone'):
            owner = ['0', '0', '0755']
        if path.is_file():
            path.chmod(int(owner[2], 8))
        fs.append((name or '/') + ' ' + ' '.join(owner))
        fc.append(re.escape('/' + name).replace('\\-', '-') + ' ' + label)
    config.write_text('\n'.join(fs) + '\n')
    contexts.write_text('\n'.join(fc) + '\n')
    image = output / (partition + '.img')
    if not full:
        shutil.copyfile(source, image)
        # Remove AVB only from the new copy before extending its EROFS filesystem.
        info = subprocess.run(['avbtool', 'info_image', '--image', str(image)], capture_output=True, text=True)
        if info.returncode == 0:
            command('avbtool', 'erase_footer', '--image', image)
    construction = ['-zlz4hc'] if full else ['--incremental=data']
    command(workspace / 'bin/Linux/x86_64/mkfs.erofs', *construction, '-T0',
            '--mount-point=/', '--fs-config-file=' + str(config), '--file-contexts=' + str(contexts),
            '-d0', image, tree)
    command('fsck.erofs', image)
    return image


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--workspace', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--android-release', required=True)
    parser.add_argument('--android-sdk', required=True)
    parser.add_argument('--host-security-patch', required=True)
    parser.add_argument('--vendor-additions', type=Path, required=True)
    args = parser.parse_args()
    workspace, output = args.workspace.resolve(), args.output.resolve()
    if not output.is_relative_to(workspace / 'out') or (output.exists() and any(output.iterdir())):
        parser.error('use a new empty workspace/out directory')
    if not re.fullmatch(r'\d{4}-\d\d-\d\d', args.host_security_patch):
        parser.error('security patch must be YYYY-MM-DD')
    if not args.android_release.isdecimal() or not args.android_sdk.isdecimal():
        parser.error('wrapper release/SDK must be numeric')
    validation = json.loads((workspace / 'out/component-validation/validation.json').read_text())
    assert validation['passed']
    for name, expected in validation['artifact_sha256'].items():
        if digest(workspace / name) != expected:
            raise RuntimeError('component changed after validation: ' + name)
    audit = json.loads((workspace / 'out/stock-dsu-audit/report.json').read_text())
    for name, record in audit['images'].items():
        print('Verifying original', name, flush=True)
        if digest(workspace / name) != record['sha256']:
            raise RuntimeError('original input changed: ' + name)
    vendor = json.loads((args.vendor_additions / 'manifest.json').read_text())
    assert vendor['base'] == 'op13_stock/vendor.img' and not vendor['whole_donor_vendor_used']
    if vendor['unresolved_needed']:
        raise RuntimeError('unresolved native vendor DT_NEEDED dependencies')
    for record in vendor['files']:
        if digest(args.vendor_additions / 'tree' / record['path']) != record['sha256']:
            raise RuntimeError('vendor addition changed: ' + record['path'])
    output.mkdir(parents=True, exist_ok=True)
    stage = output / 'system-patch'
    for name in ('system/bin', 'system/etc', 'system/lib64', 'hmos/modules', 'hmos/display', 'hmos/policy',
                 'hmos/init-modules/autorun', 'hmos/init-modules/reboot', 'hmos/empty', 'odm',
                 'product', 'system_ext', 'metadata', 'vendor_dlkm', 'odm_dlkm', 'system_dlkm',
                 'my_product', 'my_manifest', 'my_region', 'my_stock', 'my_carrier', 'my_heytap',
                 'my_bigball', 'my_engineering'):
        (stage / name).mkdir(parents=True, exist_ok=True)
    shutil.copy2(workspace / 'out/op13-bootstrap/init', stage / 'system/bin/init')
    shutil.copyfile(workspace / 'out/stock-dsu-audit/donor-init', stage / 'system/bin/hmos-native-init')
    shutil.copytree(workspace / 'out/op13-startup', stage / 'hmos/startup', symlinks=True)
    shutil.copyfile(workspace / 'out/op13-startup/mask-init-dirs', stage / 'hmos/mask-init-dirs')
    shutil.copytree(workspace / 'out/op13-ui', stage / 'hmos/ui')
    for name in ('libdisplay_buffer_vdi_impl.z.so', 'libdisplay_composer_vdi_impl.z.so'):
        shutil.copyfile(workspace / 'out/op13-display' / name, stage / 'hmos/display' / name)
    for name in ('policy.31', 'developer_policy'):
        shutil.copyfile(workspace / 'out/op13-policy' / name, stage / 'hmos/policy' / name)
    for name in ('libselinuxadp.z.so', 'libinit_context.z.so'):
        shutil.copyfile(workspace / 'work/inspect/system/system/lib64/init' / name, stage / 'hmos/init-modules' / name)
    storage = stage / 'system/bin/storage_daemon'
    shutil.copyfile(workspace / 'work/inspect/system/system/bin/storage_daemon', storage)
    previous = subprocess.check_output(['patchelf', '--print-needed', str(storage)], text=True).splitlines()
    command('patchelf', '--page-size', '4096', '--add-needed', 'libop13_local_storage.so', storage)
    after = subprocess.check_output(['patchelf', '--print-needed', str(storage)], text=True).splitlines()
    assert after == ['libop13_local_storage.so'] + previous
    shutil.copyfile(workspace / 'out/op13-storage/libop13_local_storage.so', stage / 'system/lib64/libop13_local_storage.so')
    shutil.copyfile(workspace / 'work/inspect/sweet-oobedone', stage / 'system/bin/hmos-oobedone')
    modules = json.loads((workspace / 'out/stock-dsu-audit/graphics-modules.json').read_text())
    assert not modules['conflicting_module_bytes'] and not modules['unresolved_hard_dependencies']
    order = []
    for entry in modules['closure']:
        if not re.fullmatch(r'[A-Za-z0-9_]+', entry['name']) or digest(workspace / entry['path']) != entry['sha256']:
            raise RuntimeError('module changed: ' + entry['name'])
        shutil.copyfile(workspace / entry['path'], stage / 'hmos/modules' / (entry['name'] + '.ko'))
        order.append(entry['name'])
    (stage / 'hmos/modules/load-order').write_text('\n'.join(order) + '\n')
    props = ('# OP13 port DSU compatibility metadata; not HMOS Android security fixes.\n'
        'ro.product.cpu.abi=arm64-v8a\nro.product.cpu.abilist=arm64-v8a\n'
        f'ro.system.build.version.release={args.android_release}\nro.system.build.version.sdk={args.android_sdk}\n'
        f'ro.system.build.version.security_patch={args.host_security_patch}\n')
    (stage / 'system/build.prop').write_text(props)
    (stage / 'system/etc/prop.default').write_text(props)
    owners, labels = metadata(workspace, 'system', 'system')
    # Android SwitchRoot moves its early mounts into the new read-only root.
    # Their mountpoint directories must exist before the system image boots.
    scaffold = {'debug_ramdisk', 'second_stage_resources', 'first_stage_ramdisk'}
    scaffold |= {path.name for path in (workspace / 'op13_stock/system').iterdir()
                 if path.is_dir() and path.name not in owners}
    for name in sorted(scaffold):
        if not re.fullmatch(r'[A-Za-z0-9_+-]+', name):
            raise RuntimeError('invalid stock root scaffold name')
        (stage / name).mkdir(exist_ok=True)
    images = {'system': patch_image(workspace, output, 'system', workspace / 'hmos5/system.img', stage, owners, labels, [
        ('/hmos/startup(/.*)?', 'system_etc_file'), ('/hmos/startup/vendor-init(/.*)?', 'vendor_etc_file'),
        ('/hmos/startup/hdf_default\\.hcb', 'chip_prod_file'), ('/hmos/display(/.*)?', 'vendor_lib_file'),
        ('/hmos/ui(/.*)?', 'sys_prod_file'), ('/hmos/policy(/.*)?', 'vendor_etc_file'),
        ('/hmos/init-modules(/.*)?', 'system_lib_file'), ('/system/bin/(init|hmos-native-init)', 'init_exec'),
        ('/system/bin/hmos-oobedone', 'system_bin_file'), ('/system/lib64/libop13_local_storage\\.so', 'system_lib_file')])}
    vendor_stage = output / 'vendor-patch'
    shutil.copytree(args.vendor_additions / 'tree', vendor_stage)
    owners, labels = metadata(workspace, 'stock', 'vendor')
    native_owners, native_labels = metadata(workspace, 'donor', 'vendor')
    for record in vendor['files']:
        owners[record['path']] = record['metadata']
    # Exact native labels for additions; original OP13 paths retain their
    # filesystem metadata. Stock SELinux contexts are not an OHOS policy port.
    specials = []
    for directory in vendor_stage.rglob('*'):
        if directory.is_dir():
            path = '/' + str(directory.relative_to(vendor_stage))
            label = next((value for pattern, value in native_labels if re.fullmatch(pattern, path)), None)
            if label:
                specials.append((re.escape(path), label.split(':')[2]))
    for record in vendor['files']:
        path = '/' + record['path']
        label = next(value for pattern, value in native_labels if re.fullmatch(pattern, path))
        specials.append((re.escape(path), label.split(':')[2]))
    images['vendor'] = patch_image(workspace, output, 'vendor', workspace / 'op13_stock/vendor.img',
                                  vendor_stage, owners, labels, specials, full=True)
    sources = {'odm': 'op13_stock/odm.img', 'sys_prod': 'hmos5/sys_prod.img',
        'chip_prod': 'hmos5/chip_prod.img', 'cust': 'hmos5/cust.img', 'patch_hw': 'hmos5/patch.img'}
    for name, source in sources.items():
        images[name] = output / (name + '.img')
        shutil.copyfile(workspace / source, images[name])
        if subprocess.run(['avbtool', 'info_image', '--image', str(images[name])], capture_output=True).returncode == 0:
            command('avbtool', 'erase_footer', '--image', images[name])
    key = output / 'development-key.pem'
    command('openssl', 'genpkey', '-algorithm', 'RSA', '-pkeyopt', 'rsa_keygen_bits:4096', '-out', key)
    key.chmod(0o600)
    records = {}
    for name, image in images.items():
        print('Signing and checking separate', name, flush=True)
        size = ((int(image.stat().st_size * 1.02) + 4 * 1024**2 - 1) // (4 * 1024**2)) * (4 * 1024**2)
        command('avbtool', 'add_hashtree_footer', '--image', image, '--partition_name', name,
            '--partition_size', size, '--do_not_generate_fec', '--hash_algorithm', 'sha256',
            '--algorithm', 'SHA256_RSA4096', '--key', key,
            '--prop', 'com.android.build.' + name + '.os_version:' + args.android_release,
            '--prop', 'com.android.build.' + name + '.security_patch:' + args.host_security_patch)
        command('avbtool', 'verify_image', '--image', image, '--key', key)
        command('fsck.erofs', image)
        records[name] = {'file': image.name, 'bytes': image.stat().st_size, 'sha256': digest(image)}
    package = output / 'hmos5-op13-dsu.zip'
    print('Writing multipart DSU ZIP', flush=True)
    with zipfile.ZipFile(package, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=1, allowZip64=True) as archive:
        for name, image in images.items():
            archive.write(image, image.name)
    with zipfile.ZipFile(package) as archive:
        assert archive.namelist() == [image.name for image in images.values()]
        assert archive.testzip() is None
    report = {'status': 'experimental-op13-only-multipart-dsu', 'phone_boot_tested': False,
        'kernel_modified': False, 'physical_partitions_written': False,
        'vendor_base': 'op13_stock/vendor.img', 'whole_donor_vendor_used': False,
        'odm_filesystem_base': 'op13_stock/odm.img; filesystem unchanged, development AVB footer on output copy',
        'partitions': records, 'zip': {'file': package.name, 'bytes': package.stat().st_size, 'sha256': digest(package)},
        'component_validation': validation, 'vendor_additions': vendor,
        'android_compatibility_metadata': {'release': args.android_release, 'sdk': args.android_sdk,
            'security_patch': args.host_security_patch, 'does_not_assert_hmos_android_security_fixes': True},
        'arb': 'No firmware/bootloader/vbmeta/boot/vendor_boot/dtbo images included. Device rollback state and AVB acceptance unverified.',
        'stock_vendor_existing_file_contents_replaced': False,
        'vendor_directory_labels_adapted_for_native_policy': True,
        'known_unverified': ['Android first-stage mounting all seven DSU partitions',
            'BPF attachment, native policy and additional denials', 'module/firmware and display hardware behavior',
            'missing donor libinput_driver.z.so; native input and touch need investigation',
            'access-token semantics, fscrypt, code signing, full SceneBoard startup and peripherals']}
    (output / 'manifest.json').write_text(json.dumps(report, indent=2) + '\n')
    (output / 'SHA256SUMS').write_text(''.join(record['sha256'] + '  ' + record['file'] + '\n'
        for record in list(records.values()) + [report['zip']]))
    print(json.dumps({'package': str(package), 'partitions': list(images), 'phone_boot_tested': False}, indent=2))


if __name__ == '__main__':
    main()
