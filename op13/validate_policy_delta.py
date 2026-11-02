#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Reject policy deltas outside the private BinderFS and explicit DRM changes."""
from collections import Counter
import argparse
import json
from pathlib import Path
import re


def expression(line):
    stack = [[]]
    for token in re.findall(r'"(?:\\.|[^"\\])*"|[()]|[^\s()]+', line):
        if token == '(':
            child = []
            stack[-1].append(child)
            stack.append(child)
        elif token == ')':
            stack.pop()
        else:
            stack[-1].append(token)
    if len(stack) != 1 or len(stack[0]) != 1:
        raise ValueError('malformed expression')
    return stack[0][0]


def ioctl_numbers(value):
    if isinstance(value, str):
        return {int(value, 0)}
    if value and value[0] == 'range':
        return set(range(int(value[1], 0), int(value[2], 0) + 1))
    return set().union(*(ioctl_numbers(item) for item in value))


def validate(before, after):
    old, new = Counter(before.splitlines()), Counter(after.splitlines())
    added, removed = list((new-old).elements()), list((old-new).elements())
    maps = {'composer_host', 'render_service'}
    extra = {'allocator_host': {0x6411}, 'composer_host': {0x64b3},
             'render_service': {0x6400, 0x640c, 0x64b3}}
    roots = {'"/"': 'tmpfs', '"/hmos-binder"': 'dev_binder_file',
             '"/hmos-hwbinder"': 'dev_hwbinder_file', '"/hmos-vndbinder"': 'dev_vndbinder_file'}
    seen_roots = set()
    rules = {'old': {}, 'new': {}}
    for side, lines in [('old', removed), ('new', added)]:
        for line in lines:
            node = expression(line)
            if node[0] == 'genfscon' and side == 'new' and node[1] == 'binder':
                assert node[2] in roots and node[3][2] == roots[node[2]], node
                assert node[2] not in seen_roots
                seen_roots.add(node[2])
            elif node[0] == 'allow' and node[1] in maps and node[2] == 'dev_dri_file' and node[3][0] == 'chr_file':
                rules[side][('map', node[1])] = set(node[3][1])
            elif node[0] == 'allowx' and node[1] in extra and node[2] == 'dev_dri_file' and node[3][:2] == ['ioctl', 'chr_file']:
                key = ('ioctl', node[1])
                rules[side].setdefault(key, set()).update(ioctl_numbers(node[3][2]))
            else:
                raise AssertionError('unapproved policy change: ' + line)
    assert seen_roots == set(roots), seen_roots
    changes = []
    for key in rules['old'].keys() | rules['new'].keys():
        previous, current = rules['old'].get(key, set()), rules['new'].get(key, set())
        assert previous <= current, key
        permitted = {'map'} if key[0] == 'map' else extra[key[1]]
        assert current - previous <= permitted, (key, current-previous)
        changes.append({'kind': key[0], 'domain': key[1], 'added': sorted(current-previous)})
    return {'passed': True, 'binderfs_labels_added': 4, 'drm_changes': changes,
            'all_other_decompiled_statements_unchanged': True,
            'kernel_load_tested': False, 'source_neverallow_validation_performed': False}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('before', type=Path)
    parser.add_argument('after', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    result = validate(args.before.read_text(), args.after.read_text())
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))
