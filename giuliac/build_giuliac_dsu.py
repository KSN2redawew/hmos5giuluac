#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Build the OnePlus Ace 5 (giuliac / PKG110) HarmonyOS NEXT 5 DSU port."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import zipfile

BLOCK_SIZE = 4096


def digest(path):
    result = hashlib.sha256()
    with open(path, 'rb') as f:
        for chunk in iter(lambda: f.read(4 * 1024 * 1024), b''):
            result.update(chunk)
    return result.hexdigest()


def command(*args, cwd=None):
    print('+', ' '.join(str(a) for a in args), flush=True)
    subprocess.run([str(a) for a in args], check=True, cwd=cwd)


def sign_image(image, name, key, compatibility):
    size = ((int(image.stat().st_size * 1.02) + 4 * 1024**2 - 1) // (4 * 1024**2)) * (4 * 1024**2)
    # Erase existing footer if present
    subprocess.run(['avbtool', 'erase_footer', '--image', str(image)], capture_output=True)
    command('avbtool', 'add_hashtree_footer', '--image', str(image), '--partition_name', name,
            '--partition_size', str(size), '--do_not_generate_fec', '--hash_algorithm', 'sha256',
            '--algorithm', 'SHA256_RSA4096', '--key', str(key),
            '--prop', f'com.android.build.{name}.os_version:{compatibility["release"]}',
            '--prop', f'com.android.build.{name}.security_patch:{compatibility["security_patch"]}')
    return {'file': image.name, 'bytes': image.stat().st_size, 'sha256': digest(image)}


def write_package(package_path, images, manifest_path):
    print(f'Compressing into {package_path.name} with 16 parallel threads (7z)...', flush=True)
    if package_path.exists():
        package_path.unlink()
    # 7z from working dir of output so filenames inside zip are relative (system.img, etc.)
    cmd = ['7z', 'a', '-tzip', '-mmt=16', '-mx=1', str(package_path)]
    for img in images.values():
        cmd.append(img.name)
    cmd.append(manifest_path.name)
    command(*cmd, cwd=package_path.parent)
    return {'file': package_path.name, 'bytes': package_path.stat().st_size, 'sha256': digest(package_path)}


def build_binaries(repo_dir, out_dir):
    out_dir.mkdir(parents=True, exist_ok=True)
    init_bin = out_dir / 'init'
    storage_lib = out_dir / 'libgiuliac_local_storage.so'
    
    print('Compiling giuliac/dsu_bootstrap.c for aarch64-linux-gnu (static)...', flush=True)
    command('aarch64-linux-gnu-gcc', '-static', '-O2', '-Wall', '-Wextra',
            repo_dir / 'giuliac/dsu_bootstrap.c', '-o', init_bin)
    command('aarch64-linux-gnu-strip', init_bin)

    print('Compiling giuliac/local_storage.c for aarch64-linux-gnu (shared)...', flush=True)
    command('aarch64-linux-gnu-gcc', '-shared', '-fPIC', '-O2', '-Wall', '-Wextra',
            repo_dir / 'giuliac/local_storage.c', '-ldl', '-o', storage_lib)
    command('aarch64-linux-gnu-strip', storage_lib)
    
    return init_bin, storage_lib


def extract_donor_init_and_storage(extract_bin, donor_system_img, out_dir):
    temp_extract = out_dir / 'temp_donor_extract'
    temp_extract.mkdir(parents=True, exist_ok=True)
    command(extract_bin, '-i', donor_system_img, '-X', '/system/bin/init', '-o', temp_extract, '-f')
    command(extract_bin, '-i', donor_system_img, '-X', '/system/bin/storage_daemon', '-o', temp_extract, '-f')
    
    donor_init = temp_extract / 'system/system/bin/init'
    donor_storage = temp_extract / 'system/system/bin/storage_daemon'
    if not donor_init.exists():
        raise FileNotFoundError(f'donor init not found at {donor_init}')
    if not donor_storage.exists():
        raise FileNotFoundError(f'donor storage_daemon not found at {donor_storage}')
    return donor_init, donor_storage


def generate_load_order(vdlkm_tree, out_file):
    lines = ['# Module load order for OnePlus Ace 5 (giuliac / pineapple)']
    modules_dir = vdlkm_tree / 'lib/modules'
    if (modules_dir / 'modules.load').exists():
        for line in (modules_dir / 'modules.load').read_text().splitlines():
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            name = Path(line).stem.replace('-', '_')
            path = f'/vendor_dlkm/lib/modules/{Path(line).name}'
            lines.append(f'{name} {path}')
    else:
        for ko in sorted(modules_dir.glob('*.ko')):
            name = ko.stem.replace('-', '_')
            lines.append(f'{name} /vendor_dlkm/lib/modules/{ko.name}')
    out_file.write_text('\n'.join(lines) + '\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument('--donor-dir', type=Path, default=Path('/mnt/f/fowa/HarmonyOS-pura-70/extracted'))
    parser.add_argument('--super-dir', type=Path, default=Path('/mnt/f/fowa/super'))
    parser.add_argument('--output', type=Path, default=Path('/mnt/f/HOS-5-giuliac-port/out/giuliac-dsu'))
    parser.add_argument('--android-release', default='17')
    parser.add_argument('--android-sdk', default='37')
    parser.add_argument('--host-security-patch', default='2026-09-01')
    args = parser.parse_args()

    repo = args.repo.resolve()
    donor_dir = args.donor_dir.resolve()
    super_dir = args.super_dir.resolve()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    extract_bin = repo / 'tools/bin/extract.erofs'
    mkfs_bin = repo / 'tools/bin/mkfs.erofs'

    compatibility = {
        'release': args.android_release,
        'sdk': args.android_sdk,
        'security_patch': args.host_security_patch
    }

    # 1. Compile bootstrap and storage library
    init_bin, storage_lib = build_binaries(repo, output / 'bin')

    # 2. Extract donor init and storage daemon
    donor_init, donor_storage = extract_donor_init_and_storage(extract_bin, donor_dir / 'system.img', output)

    # 3. Prepare system-patch staging tree
    print('Preparing system staging directory...', flush=True)
    stage = output / 'system-stage'
    shutil.rmtree(stage, ignore_errors=True)
    stage.mkdir(parents=True, exist_ok=True)

    for d in ('system/bin', 'system/etc', 'system/lib', 'system/lib64',
              'hmos/modules', 'hmos/display', 'hmos/ui', 'hmos/empty',
              'odm', 'product', 'system_ext', 'metadata', 'vendor_dlkm',
              'system_dlkm', 'chip_prod', 'sys_prod', 'cust', 'patch_hw',
              'my_product', 'my_manifest', 'my_region', 'my_stock',
              'my_carrier', 'my_heytap', 'my_bigball', 'my_engineering'):
        (stage / d).mkdir(parents=True, exist_ok=True)

    # Copy bootstrap and donor inits
    shutil.copy2(init_bin, stage / 'system/bin/init')
    (stage / 'system/bin/init').chmod(0o755)
    shutil.copy2(donor_init, stage / 'system/bin/hmos-native-init')
    (stage / 'system/bin/hmos-native-init').chmod(0o755)

    # Patch storage_daemon
    storage_dest = stage / 'system/bin/storage_daemon'
    shutil.copy2(donor_storage, storage_dest)
    command('patchelf', '--page-size', '4096', '--add-needed', 'libgiuliac_local_storage.so', storage_dest)
    storage_dest.chmod(0o755)
    shutil.copy2(storage_lib, stage / 'system/lib64/libgiuliac_local_storage.so')
    (stage / 'system/lib64/libgiuliac_local_storage.so').chmod(0o644)

    # OOBE bypass helper
    oobe_helper = repo / 'patches/50-remove-setup-wizard/files/bin/sweet-oobedone'
    if oobe_helper.exists():
        shutil.copy2(oobe_helper, stage / 'system/bin/hmos-oobedone')
        (stage / 'system/bin/hmos-oobedone').chmod(0o755)

    # Display and UI configs
    shutil.copy2(repo / 'giuliac/display_manager_config.xml', stage / 'hmos/ui/display_manager_config.xml')
    shutil.copy2(repo / 'giuliac/hgm_policy_config.xml', stage / 'hmos/ui/hgm_policy_config.xml')

    display_vdi_buffer = repo / 'patches/10-display/files/lib64/libdisplay_buffer_vdi_impl.z.so'
    display_vdi_composer = repo / 'patches/10-display/files/lib64/libdisplay_composer_vdi_impl.z.so'
    if display_vdi_buffer.exists():
        shutil.copy2(display_vdi_buffer, stage / 'hmos/display/libdisplay_buffer_vdi_impl.z.so')
    if display_vdi_composer.exists():
        shutil.copy2(display_vdi_composer, stage / 'hmos/display/libdisplay_composer_vdi_impl.z.so')

    # Modules load order
    vdlkm_extract = output / 'vendor_dlkm_extract'
    vdlkm_extract.mkdir(parents=True, exist_ok=True)
    command(extract_bin, '-i', super_dir / 'images/vendor_dlkm_a.img', '-x', '-o', vdlkm_extract, '-f')
    generate_load_order(vdlkm_extract, stage / 'hmos/modules/load-order')

    # Symlink /system/lib/modules -> /system_dlkm/lib/modules
    modules_link = stage / 'system/lib/modules'
    if modules_link.exists() or modules_link.is_symlink():
        modules_link.unlink()
    modules_link.symlink_to('/system_dlkm/lib/modules')

    # Props for OnePlus Ace 5 (PKG110 / giuliac)
    props = (
        '# OnePlus Ace 5 (PKG110) HarmonyOS NEXT 5 DSU compatibility metadata\n'
        'ro.product.model=PKG110\n'
        'ro.product.name=PKG110\n'
        'ro.product.device=OP5D2BL1\n'
        'ro.product.cpu.abi=arm64-v8a\n'
        'ro.product.cpu.abilist=arm64-v8a\n'
        f'ro.system.build.version.release={compatibility["release"]}\n'
        f'ro.system.build.version.sdk={compatibility["sdk"]}\n'
        f'ro.system.build.version.security_patch={compatibility["security_patch"]}\n'
    )
    (stage / 'system/build.prop').write_text(props)
    (stage / 'system/etc/prop.default').write_text(props)

    # Scaffold root dirs from stock system
    stock_system = super_dir / 'system_a'
    if stock_system.exists():
        for item in stock_system.iterdir():
            if item.is_dir() and re.fullmatch(r'[A-Za-z0-9_+-]+', item.name):
                (stage / item.name).mkdir(parents=True, exist_ok=True)

    # Generate fs_config and file_contexts for system-stage
    fs_lines = []
    fc_lines = []
    for path in [stage] + sorted(stage.rglob('*')):
        rel = '' if path == stage else str(path.relative_to(stage))
        mode = '0755' if path.is_dir() else '0644'
        if rel in ('system/bin/init', 'system/bin/hmos-native-init',
                   'system/bin/storage_daemon', 'system/bin/hmos-oobedone'):
            mode = '0755'
        fs_lines.append(f'/{rel} 0 0 {mode}')
        label = 'u:object_r:system_file:s0'
        if rel.startswith('hmos/ui'):
            label = 'u:object_r:sys_prod_file:s0'
        elif rel.startswith('hmos/display'):
            label = 'u:object_r:vendor_lib_file:s0'
        elif rel.startswith('system/bin/init') or rel.startswith('system/bin/hmos-native-init'):
            label = 'u:object_r:init_exec:s0'
        escaped_pattern = re.escape('/' + rel).replace(r'\-', '-')
        fc_lines.append(f'{escaped_pattern} {label}')

    sys_fs_config = output / 'system-fs-config'
    sys_fc = output / 'system-file-contexts'
    sys_fs_config.write_text('\n'.join(fs_lines) + '\n')
    sys_fc.write_text('\n'.join(fc_lines) + '\n')

    # 4. Create system.img using incremental erofs
    print('Building modified system.img incrementally...', flush=True)
    out_system_img = output / 'system.img'
    shutil.copyfile(donor_dir / 'system.img', out_system_img)
    command(mkfs_bin, '--incremental=data', '-T0', '--mount-point=/',
            f'--fs-config-file={sys_fs_config}', f'--file-contexts={sys_fc}',
            '-d0', out_system_img, stage)
    command('fsck.erofs', str(out_system_img))

    # 5. Build vendor.img from stock vendor_a + native HMOS additions
    print('Building customized vendor.img...', flush=True)
    vendor_stage = output / 'vendor-stage'
    if not (vendor_stage / 'bin').exists():
        shutil.rmtree(vendor_stage, ignore_errors=True)
        print('  copying stock vendor_a...', flush=True)
        shutil.copytree(super_dir / 'vendor_a', vendor_stage, symlinks=True)
    else:
        print('  reusing existing vendor-stage directory...', flush=True)

    # Add native display VDI
    (vendor_stage / 'lib64').mkdir(parents=True, exist_ok=True)
    if display_vdi_buffer.exists():
        shutil.copy2(display_vdi_buffer, vendor_stage / 'lib64/libdisplay_buffer_vdi_impl.z.so')
    if display_vdi_composer.exists():
        shutil.copy2(display_vdi_composer, vendor_stage / 'lib64/libdisplay_composer_vdi_impl.z.so')

    # Add mesa software GL libs
    mesa_dir = repo / 'patches/11-gpu-mesa/files/lib64/chipsetsdk'
    if mesa_dir.exists():
        (vendor_stage / 'lib64/chipsetsdk').mkdir(parents=True, exist_ok=True)
        for f in mesa_dir.glob('*.so*'):
            shutil.copy2(f, vendor_stage / 'lib64/chipsetsdk' / f.name)

    # Generate normalized fs_config and file_contexts for vendor-stage
    stock_vendor_fs_config = super_dir / 'config/vendor_a_fs_config'
    stock_vendor_fc = super_dir / 'config/vendor_a_file_contexts'
    
    vendor_owners = {}
    if stock_vendor_fs_config.exists():
        for line in stock_vendor_fs_config.read_text().splitlines():
            line = line.strip()
            if not line:
                continue
            parts = line.split()
            clean_path = parts[0].removeprefix('vendor_a/').removeprefix('vendor/').lstrip('/')
            vendor_owners[clean_path] = parts[1:]

    vendor_labels = []
    if stock_vendor_fc.exists():
        for line in stock_vendor_fc.read_text().splitlines():
            line = line.strip()
            if not line:
                continue
            parts = line.split()
            clean_pattern = parts[0].removeprefix('/vendor_a/').removeprefix('/vendor/').removeprefix('/vendor_a').removeprefix('/vendor').lstrip('/')
            vendor_labels.append((clean_pattern, parts[1]))

    v_fs_lines = []
    v_fc_lines = []
    for path in [vendor_stage] + sorted(vendor_stage.rglob('*')):
        rel = '' if path == vendor_stage else str(path.relative_to(vendor_stage))
        def_mode = '0755' if path.is_dir() else '0644'
        meta = vendor_owners.get(rel, ['0', '2000' if path.is_dir() else '0', def_mode])
        v_fs_lines.append(f'/{rel} {" ".join(meta)}')

        label = 'u:object_r:vendor_file:s0'
        if rel.startswith('apex'):
            label = 'u:object_r:vendor_apex_file:s0'
        elif rel.startswith('app'):
            label = 'u:object_r:vendor_app_file:s0'
        elif rel.startswith('bin'):
            label = 'u:object_r:vendor_file:s0'
        elif rel.startswith('etc/selinux'):
            label = 'u:object_r:vendor_configs_file:s0'
        elif rel.startswith('lib64'):
            label = 'u:object_r:vendor_file:s0'
        escaped_pattern = re.escape('/' + rel).replace(r'\-', '-')
        v_fc_lines.append(f'{escaped_pattern} {label}')

    vendor_fs_config = output / 'vendor-fs-config'
    vendor_fc = output / 'vendor-file-contexts'
    vendor_fs_config.write_text('\n'.join(v_fs_lines) + '\n')
    vendor_fc.write_text('\n'.join(v_fc_lines) + '\n')

    out_vendor_img = output / 'vendor.img'
    if out_vendor_img.exists():
        out_vendor_img.unlink()
    command(mkfs_bin, '-zlz4hc', '-T0', '--mount-point=/',
            f'--fs-config-file={vendor_fs_config}', f'--file-contexts={vendor_fc}',
            '-d0', out_vendor_img, vendor_stage)
    command('fsck.erofs', str(out_vendor_img))

    # 6. Copy DLKM and auxiliary images
    images = {
        'system': out_system_img,
        'vendor': out_vendor_img,
        'vendor_dlkm': output / 'vendor_dlkm.img',
        'system_dlkm': output / 'system_dlkm.img',
        'sys_prod': output / 'sys_prod.img',
        'chip_prod': output / 'chip_prod.img',
        'cust': output / 'cust.img',
        'patch_hw': output / 'patch_hw.img',
    }

    print('Copying unchanged DLKM and donor partitions...', flush=True)
    shutil.copyfile(super_dir / 'images/vendor_dlkm_a.img', images['vendor_dlkm'])
    shutil.copyfile(super_dir / 'images/system_dlkm_a.img', images['system_dlkm'])
    shutil.copyfile(donor_dir / 'sys_prod.img', images['sys_prod'])
    shutil.copyfile(donor_dir / 'chip_prod.img', images['chip_prod'])
    shutil.copyfile(donor_dir / 'cust.img', images['cust'])
    shutil.copyfile(donor_dir / 'patch.img', images['patch_hw'])

    # 7. Generate RSA key and sign all 8 images with AVB footers
    key_path = output / 'development-key.pem'
    if not key_path.exists():
        print('Generating RSA-4096 AVB signing key...', flush=True)
        command('openssl', 'genpkey', '-algorithm', 'RSA', '-pkeyopt', 'rsa_keygen_bits:4096', '-out', key_path)
        key_path.chmod(0o600)

    signed_records = {}
    for name, img_path in images.items():
        print(f'Signing {name} ({img_path.name})...', flush=True)
        signed_records[name] = sign_image(img_path, name, key_path, compatibility)

    # 8. Write manifest and package into multipart DSU ZIP with 16 threads
    manifest_file = output / 'manifest.json'
    manifest = {
        'schema_version': 1,
        'name': 'HarmonyOS NEXT 5.1 (OnePlus Ace 5 / giuliac)',
        'source': 'Huawei Pura 70 OTA (205.1.0.128)',
        'device': 'OnePlus Ace 5 (PKG110 / OP5D2BL1)',
        'platform': 'Snapdragon 8 Gen 3 (pineapple / SM8650)',
        'android_metadata': compatibility,
        'partitions': [
            {'name': name, 'file_name': img_path.name}
            for name, img_path in images.items()
        ]
    }
    manifest_file.write_text(json.dumps(manifest, indent=2) + '\n')

    pkg_file = output / 'hmos5-giuliac-dsu.zip'
    package_record = write_package(pkg_file, images, manifest_file)

    sha_text = ''.join(f'{rec["sha256"]}  {rec["file"]}\n' for rec in list(signed_records.values()) + [package_record])
    (output / 'SHA256SUMS').write_text(sha_text)

    print('=' * 60, flush=True)
    print('BUILD SUCCESSFUL!', flush=True)
    print(f'Package: {pkg_file} ({package_record["bytes"] / 1e6:.1f} MB)', flush=True)
    print('=' * 60, flush=True)


if __name__ == '__main__':
    main()
