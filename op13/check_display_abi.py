#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Compare relocated composer vtable slots in the donor and candidate ELF64."""
import argparse
import hashlib
import json
from pathlib import Path
import struct


def table(path):
    data = path.read_bytes()
    if data[:6] != b'\x7fELF\x02\x01':
        raise ValueError('expected little-endian ELF64')
    shoff = struct.unpack_from('<Q', data, 40)[0]
    entsize, count = struct.unpack_from('<HH', data, 58)
    sections = [struct.unpack_from('<IIQQQQIIQQ', data, shoff + i * entsize)
                for i in range(count)]
    symbols = {}
    by_address = {}
    for index, section in enumerate(sections):
        if section[1] not in (2, 11):
            continue
        strings = sections[section[6]]
        names = data[strings[4]:strings[4] + strings[5]]
        rows = []
        for off in range(section[4], section[4] + section[5], section[9]):
            name, info, other, shndx, value, size = struct.unpack_from('<IBBHQQ', data, off)
            text = names[name:names.find(b'\0', name)].decode()
            rows.append((text, value, size))
            if shndx and text:
                by_address.setdefault(value, set()).add(text)
        symbols[index] = rows
    marker = '_ZTVN4OHOS3HDI7DISPLAY22DisplayComposerVdiImplE'
    candidates = [row for rows in symbols.values() for row in rows if row[0] == marker]
    if not candidates:
        raise ValueError('composer vtable not found')
    _, start, size = candidates[0]
    values = [set() for _ in range(size // 8)]
    for section in sections:
        if section[1] != 4:
            continue
        for off in range(section[4], section[4] + section[5], section[9]):
            address, info, addend = struct.unpack_from('<QQq', data, off)
            if not start <= address < start + size:
                continue
            kind, symbol = info & 0xffffffff, info >> 32
            slot = (address - start) // 8
            if kind == 1027:  # R_AARCH64_RELATIVE
                values[slot] = by_address.get(addend, {'unresolved-relative:' + hex(addend)})
            elif kind == 257:  # R_AARCH64_ABS64
                values[slot] = {symbols[section[6]][symbol][0]}
            else:
                raise ValueError('unexpected vtable relocation type ' + str(kind))
    return values


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('donor', type=Path)
    parser.add_argument('candidate', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    donor, candidate = table(args.donor), table(args.candidate)
    differences = [{'slot': i, 'donor': sorted(a), 'candidate': sorted(b)}
                   for i, (a, b) in enumerate(zip(donor, candidate))
                   if a != b and not (a & b)]
    passed = len(donor) == len(candidate) and not differences
    result = {'passed': passed, 'donor_slots': len(donor), 'candidate_slots': len(candidate),
              'differences': differences,
              'donor_sha256': hashlib.sha256(args.donor.read_bytes()).hexdigest(),
              'candidate_sha256': hashlib.sha256(args.candidate.read_bytes()).hexdigest(),
              'scope': 'Relocated virtual function slot names/order only; no runtime or hardware claim'}
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))
    if not passed:
        raise SystemExit(1)


if __name__ == '__main__':
    main()
