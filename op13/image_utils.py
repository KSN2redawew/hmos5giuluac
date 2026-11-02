#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Read and check EROFS/ext4 images without changing their contents."""
import struct
import subprocess
import hashlib


def digest(path):
    result = hashlib.sha256()
    with path.open('rb') as file:
        for chunk in iter(lambda: file.read(4 * 1024 * 1024), b''):
            result.update(chunk)
    return result.hexdigest()


def filesystem_info(path):
    with path.open('rb') as file:
        file.seek(1024)
        header = file.read(1024)
    if header[:4] == b'\xe2\xe1\xf5\xe0':
        kind = 'erofs'
        block_size = 1 << header[12]
        blocks = struct.unpack_from('<I', header, 36)[0]
    elif header[56:58] == b'\x53\xef':
        kind = 'ext4'
        block_size = 1024 << struct.unpack_from('<I', header, 24)[0]
        blocks = struct.unpack_from('<I', header, 4)[0]
        if struct.unpack_from('<I', header, 96)[0] & 0x80:
            blocks |= struct.unpack_from('<I', header, 336)[0] << 32
    else:
        raise ValueError('unsupported filesystem: ' + str(path))
    size = blocks * block_size
    if block_size < 1024 or block_size > 65536 or not 0 < size <= path.stat().st_size:
        raise ValueError('invalid filesystem bounds: ' + str(path))
    return {'type': kind, 'bytes': size, 'block_size': block_size}


def check_filesystem(path):
    info = filesystem_info(path)
    command = ['fsck.erofs', str(path)] if info['type'] == 'erofs' else ['e2fsck', '-fn', str(path)]
    subprocess.run(command, check=True)
    return info
