#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Inventory the actual donor/stock inputs without using hmos2dodge code.

Extracts only configuration and selected runtime files into an output tree.
The generated report is evidence for porting, not a bootability certificate.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import zipfile


def run(*args):
    return subprocess.check_output([str(a) for a in args], text=True,
                                   stderr=subprocess.STDOUT)


def sha256(path):
    digest = hashlib.sha256()
    with path.open('rb') as source:
        for block in iter(lambda: source.read(4 * 1024 * 1024), b''):
            digest.update(block)
    return digest.hexdigest()


def inventory_configs(tree):
    services, jobs, imports, errors = [], [], [], []
    # cfg files can contain comments in some donor releases. Keep failures
    # explicit rather than silently presenting an incomplete graph as complete.
    for path in sorted(tree.rglob('*.cfg')):
        relative = path.relative_to(tree)
        if 'init' not in relative.parts and not path.name.startswith('init.'):
            continue
        try:
            data = json.loads(path.read_text())
        except (ValueError, UnicodeError) as error:
            errors.append({'file': str(path.relative_to(tree)), 'error': str(error)})
            continue
        if not isinstance(data, dict):
            continue
        origin = str(path.relative_to(tree))
        malformed = [key for key in ('jobs', 'services', 'import')
                     if not isinstance(data.get(key, []), list)]
        if malformed:
            errors.append({'file': origin, 'error': 'non-list fields: ' + ', '.join(malformed)})
            continue
        for service in data.get('services', []):
            if isinstance(service, dict):
                services.append({'source': origin, 'definition': service})
            else:
                errors.append({'file': origin, 'error': 'non-object service'})
        for job in data.get('jobs', []):
            if isinstance(job, dict) and isinstance(job.get('cmds', []), list):
                jobs.append({'source': origin, 'definition': job})
            else:
                errors.append({'file': origin, 'error': 'malformed job'})
        for target in data.get('import', []):
            imports.append({'source': origin, 'target': target})
    hazards = []
    for job in jobs:
        for command in job['definition'].get('cmds', []):
            if not isinstance(command, str):
                errors.append({'file': job['source'], 'error': 'non-string command'})
                continue
            reasons = []
            if '/dev/block/' in command:
                reasons.append('block-device reference requires DSU-specific review')
            if re.match(r'\s*(mount|mount_fstab|umount|format|mknod|insmod|reboot)\b', command):
                reasons.append('storage/kernel/boot operation requires replacement or review')
            if 'sweet' in command or 'Kirin' in command:
                reasons.append('donor hardware dependency')
            if reasons:
                hazards.append({'source': job['source'], 'job': job['definition'].get('name'),
                                'command': command, 'reasons': reasons})
    return dict(services=services, jobs=jobs, imports=imports,
                parse_errors=errors, commands_requiring_review=hazards,
                scope='init.*.cfg and init/**/*.cfg in selected trees; not a resolved execution graph')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--workspace', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    root, output = args.workspace.resolve(), args.output.resolve()
    if output == root or root in output.parents and any(
            p in output.relative_to(root).parts for p in ('hmos5', 'op13_stock')):
        parser.error('output must be separate from the original inputs')
    if output.exists() and any(output.iterdir()):
        parser.error('use a new empty output directory to avoid mixing audit runs')
    output.mkdir(parents=True, exist_ok=True)
    reference = Path(__file__).resolve().parents[1]
    report = {'schema': 1, 'upstream_commit': run('git', '-C', reference,
              'rev-parse', 'HEAD').strip(), 'images': {}, 'boot_verified': False,
              'dsu_package_built': False, 'kernel_modifications': False}
    images = sorted((root / 'hmos5').glob('*.img')) + sorted((root / 'op13_stock').glob('*.img'))
    if not images:
        parser.error('no input images')
    for image in images:
        print(f'Hashing {image.relative_to(root)}', flush=True)
        report['images'][str(image.relative_to(root))] = {
            'bytes': image.stat().st_size, 'sha256': sha256(image),
            'format': run('file', '-b', image).strip()}
    selections = [('hmos5/system.img', '/system/etc', 'donor-system-etc'),
                  ('hmos5/vendor.img', '/etc', 'donor-vendor-etc'),
                  ('op13_stock/vendor.img', '/etc', 'stock-vendor-etc'),
                  ('op13_stock/odm.img', '/etc', 'stock-odm-etc'),
                  ('hmos5/system.img', '/system/bin/init', 'donor-init'),
                  ('hmos5/system.img', '/system/lib64/chipset-pub-sdk/libipc_common.z.so', 'donor-ipc-common'),
                  ('hmos5/system.img', '/system/lib64/platformsdk/libipc_core.z.so', 'donor-ipc-core')]
    for image, path, name in selections:
        print(f'Inspecting {image}:{path}', flush=True)
        run('fsck.erofs', f'--extract={output / name}', f'--path={path}',
            '--no-preserve-owner', '--preserve-perms', root / image)
    for name in ('donor-system-etc', 'donor-vendor-etc'):
        report[name] = inventory_configs(output / name)
    report['app_listing'] = run('dump.erofs', '--ls', '--path=/system/app', root / 'hmos5/system.img')
    report['sceneboard_directory_present'] = 'SceneBoard' in report['app_listing']
    report['fstabs'] = {}
    for name in ('donor-vendor-etc', 'stock-vendor-etc', 'stock-odm-etc'):
        for path in sorted((output / name).rglob('*fstab*')):
            if path.is_file():
                report['fstabs'][f'{name}/{path.relative_to(output / name)}'] = path.read_text(errors='replace')
    # The existing Android capture provides hardware observations, not a DSU test.
    archive = root / '20261029-163450.zip'
    if archive.exists():
        with zipfile.ZipFile(archive) as capture:
            sysfs = next(n for n in capture.namelist() if n.endswith('/display-sysfs.txt'))
            props = next(n for n in capture.namelist() if n.endswith('/getprop.txt'))
            report['android_capture'] = {
                'sha256': sha256(archive),
                'display_sysfs': capture.read(sysfs).decode(),
                'selected_properties': [line for line in capture.read(props).decode().splitlines()
                    if re.match(r'\[(ro.product.model|ro.soc.model|ro.build.version.security_patch)\]', line)],
                'native_hmos_or_dsu_test': False}
    report['unresolved'] = [
        'Stock-kernel Binder/token fallback must be exercised with this HMOS5 runtime',
        'DSU first-stage handoff and persistent userdata_gsi storage isolation',
        'OP13 stock module/firmware startup and native OHOS HDI integration',
        'Graphics allocation, DRM access, KGSL and donor Mesa support on SM8750',
        'Storage service encryption/code-signing compatibility and SELinux policy',
        'Full foundation/appspawn/SceneBoard launch on the phone',
        'DSU image AVB acceptance against the installed Android build']
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(output / 'report.json')


if __name__ == '__main__':
    main()
