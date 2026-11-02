#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Prepare a full-framework init overlay for the guarded DSU bootstrap.

This does not launch init, mount filesystems, or build a flashable ROM.
The bootstrap must enforce device isolation independently of these edits.
"""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import re
import subprocess

BLOCKED_SERVICES = {'ueventd', 'watchdog_service', 'updater', 'updater_sa',
                    'update_service', 'kdump_resize', 'iSulad'}
BLOCKED_COMMANDS = {'mount_fstab', 'umount', 'format', 'mknod', 'insmod',
                    'reboot', 'swapon', 'swapoff', 'mkswap'}


def command_reason(command, disabled):
    words = command.split()
    if not words:
        return None
    if '/dev/block/' in command or '/dev/disk/' in command:
        return 'physical/unspecified block device reference'
    if words[0] in BLOCKED_COMMANDS:
        return 'storage/kernel/reboot operation owned by bootstrap'
    if words[0] == 'mount':
        # Namespaced cgroups have no physical disk source. Everything else
        # must be mounted explicitly by the DSU bootstrap or its storage layer.
        if not (len(words) >= 4 and words[1] in ('cgroup', 'cgroup2')
                and words[2] == 'none' and words[3].startswith('/dev/')):
            return 'unreviewed mount'
    if words[0] in ('write', 'chmod', 'chown', 'mkdir') and any(
            path.startswith('/sys/') for path in words[1:]):
        return 'donor sysfs mutation'
    if any(token in command for token in ('panic_on_oops', 'sysrq-trigger', 'reboot,', 'reboot.')):
        return 'reboot/panic operation'
    if words[0] in ('start', 'reset', 'stop') and len(words) > 1 and words[1] in disabled:
        return 'service excluded from this startup'
    return None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--audit', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--hdf-source', type=Path, required=True)
    parser.add_argument('--hc-gen', type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    if output.exists() and any(output.iterdir()):
        parser.error('output must be empty')
    (output / 'system-init').mkdir(parents=True, exist_ok=True)
    (output / 'vendor-init').mkdir()
    report = {'runtime_tested': False, 'device_isolation_enforced_by_this_generator': False,
              'removed_commands': [], 'removed_services': [], 'removed_imports': [],
              'preserved_services': [], 'source_sha256': {}}
    sources = [args.audit / 'donor-system-etc/init.cfg']
    sources += sorted((args.audit / 'donor-system-etc/init').glob('*.cfg'))
    parsed = []
    for source in sources:
        original = source.read_bytes()
        data = json.loads(original)
        report['source_sha256'][str(source)] = hashlib.sha256(original).hexdigest()
        parsed.append((source.name, data))
    disabled = set(BLOCKED_SERVICES)
    for name, data in parsed:
        for service in data.get('services', []):
            executable = service.get('path', [''])[0]
            if any(token in executable for token in ('/updater', '/rescue', '/flashd', '/kdump')):
                disabled.add(service['name'])
    for name, data in parsed:
        data = copy.deepcopy(data)
        for target in data.pop('import', []):
            report['removed_imports'].append({'source': name, 'target': target})
        kept = []
        for service in data.get('services', []):
            if service['name'] in disabled:
                report['removed_services'].append({'source': name, 'service': service['name']})
                continue
            if service.get('critical'):
                service['critical'][0] = 0
            report['preserved_services'].append({'source': name, 'service': service['name'],
                'uid': service.get('uid'), 'secon': service.get('secon')})
            kept.append(service)
        if 'services' in data:
            data['services'] = kept
        for job in data.get('jobs', []):
            kept_commands = []
            for command in job.get('cmds', []):
                reason = command_reason(command, disabled)
                if reason:
                    report['removed_commands'].append({'source': name, 'job': job['name'],
                                                       'command': command, 'reason': reason})
                else:
                    kept_commands.append(command)
            job['cmds'] = kept_commands
        target = output / ('init.cfg' if name == 'init.cfg' else 'system-init/' + name)
        target.write_text(json.dumps(data, indent=2) + '\n')
    # Native HDF manager, with a curated HDF host graph below. Do not import
    # the Redmi hardware start scripts or Android init .rc files as OHOS cfg.
    hdf = json.loads((args.audit / 'donor-vendor-etc/init/hdf_devmgr.cfg').read_text())
    for service in hdf.get('services', []):
        if service.get('critical'):
            service['critical'][0] = 0
    (output / 'vendor-init/hdf_devmgr.cfg').write_text(json.dumps(hdf, indent=2) + '\n')
    # The supplied donor already removes its setup wizard. Preserve its
    # existing settings helper, but run a system-labeled copy in the guest.
    donor_patch = json.loads((args.audit / 'donor-vendor-etc/init/hmos2sweet.cfg').read_text())
    helper = copy.deepcopy(next(s for s in donor_patch['services'] if s['name'] == 'sweet_oobedone'))
    helper['name'] = 'hmos_oobedone'
    helper['path'] = ['/system/bin/hmos-oobedone']
    jobs = [{'name': 'post-fs-data', 'cmds': ['start hmos_oobedone']}]
    for event in ('bootevent.bms.main.bundles.ready', 'bootevent.appfwk.ready', 'bootevent.launcher.ready'):
        jobs.append({'name': 'param:' + event + '=*', 'condition': event + '=*',
                     'cmds': ['start hmos_oobedone']})
    (output / 'system-init/op13-donor-setup.cfg').write_text(json.dumps({
        'services': [helper], 'jobs': jobs}, indent=2) + '\n')
    report['donor_setup_helper_retained'] = True
    # Extract host blocks from hc-gen's decompiled source. Braces inside quoted
    # strings and comments do not affect block nesting.
    text = args.hdf_source.read_text()
    tokens = list(re.finditer(r'"(?:\\.|[^"\\])*"|/\*.*?\*/|//[^\n]*|[{}]|[A-Za-z_]\w*', text, re.S))
    stack, blocks = [], []
    previous = None
    for token in tokens:
        value = token.group()
        if value.startswith(('"', '/*', '//')):
            continue
        if value == '{':
            stack.append((previous.group(), previous.start()))
        elif value == '}':
            name, start = stack.pop()
            if len(stack) == 2 and stack[-1][0] == 'device_info':
                blocks.append((name, text[start:token.end()]))
        previous = token
    selected = []
    hosts = {'composer_host', 'allocator_host', 'input_user_host', 'pin_auth_host', 'user_auth_host', 'power_host'}
    for name, block in blocks:
        match = re.search(r'hostName\s*=\s*"([^"]+)"', block)
        if match and match.group(1) in hosts:
            if match.group(1) == 'composer_host':
                # Kirin display-engine integration is separate from the native
                # DRM composer and must not auto-start in the OP13 process.
                block = re.sub(r'\s*displayengine_device\s*\{.*?\n\s{12}\}', '', block, flags=re.S)
            if match.group(1) == 'power_host':
                block = re.sub(r'\s*(thermal_device|thermal_daemon_device)\s*\{.*?\n\s{12}\}', '', block, flags=re.S)
            selected.append(block)
    if len(selected) != len(hosts):
        raise ValueError('expected native display/input/auth and power/battery hosts')
    hcs = 'root {\n module = "default";\n device_info {\n match_attr = "hdf_manager";\n' + '\n'.join(selected) + '\n }\n}\n'
    (output / 'hdf-op13.hcs').write_text(hcs)
    subprocess.run([str(args.hc_gen.resolve()), '-b', '-o', str(output / 'hdf_default.hcb'),
                    str(output / 'hdf-op13.hcs')], check=True)
    report['hdf_hosts'] = sorted(hosts)
    report['hdf_source_sha256'] = hashlib.sha256(args.hdf_source.read_bytes()).hexdigest()
    report['full_framework_preserved'] = all(name in {s['service'] for s in report['preserved_services']}
        for name in ('foundation', 'appspawn', 'samgr', 'render_service', 'accesstoken_service', 'storage_daemon'))
    if not report['full_framework_preserved']:
        raise ValueError('essential full-framework service was lost')
    (output / 'manifest.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'services': len(report['preserved_services']),
        'commands_removed': len(report['removed_commands']), 'hdf_hosts': report['hdf_hosts'],
        'full_framework_preserved': report['full_framework_preserved']}, indent=2))


if __name__ == '__main__':
    main()
