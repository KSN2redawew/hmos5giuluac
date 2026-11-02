#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Adapt native geometry and discrete refresh bounds from the OP13 capture."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import xml.etree.ElementTree as ET
import zipfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture', type=Path, required=True)
    parser.add_argument('--donor', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    with zipfile.ZipFile(args.capture) as archive:
        sysfs = archive.read(next(n for n in archive.namelist() if n.endswith('/display-sysfs.txt'))).decode()
        display = archive.read(next(n for n in archive.namelist() if n.endswith('/display.txt'))).decode()
    modes = sorted({tuple(map(int, m)) for m in re.findall(r'^(\d+)x(\d+)x(\d+)cmd$', sysfs, re.M)})
    dimensions = {(w, h) for w, h, _ in modes}
    if len(dimensions) != 1:
        raise ValueError('expected one native resolution in captured DRM modes')
    width, height = next(iter(dimensions))
    rates = sorted({rate for _, _, rate in modes})
    density = int(re.search(r'\bdensity (\d+)\b', display).group(1))
    cutout = re.search(r'cutoutSpec=\{([^}]+)\}', display).group(1)
    numbers = list(map(int, re.findall(r'-?\d+', cutout)))
    if len(numbers) != 8 or numbers[1] != 0 or numbers[7] != 0 or numbers[3] != numbers[5]:
        raise ValueError('expected captured rectangular native cutout')
    left, right, bottom = width // 2 + numbers[0], width // 2 + numbers[4], numbers[3]
    def read_xml(name):
        return ET.parse(args.donor / name, parser=ET.XMLParser(target=ET.TreeBuilder(insert_comments=True)))
    geometry = read_xml('display_manager_config.xml')
    for tag, value in {
        'dpi': str(density), 'cutoutArea': f'{left} 0 {right-left} {bottom}',
        'defaultDisplayCutoutPath': f'M{left} 0 L{right} 0 L{right} {bottom} L{left} {bottom} Z'
    }.items():
        node = geometry.getroot().find(tag)
        if node is None:
            raise ValueError('missing geometry element ' + tag)
        node.text = value
    policy = read_xml('hgm_policy_config.xml')
    changed = 0
    for strategy in policy.findall(".//Params[@name='refreshRate_strategy_config']/Strategy"):
        strategy.set('min', str(min(rates)))
        strategy.set('max', str(max(rates)))
        strategy.set('dynamicMode', '0')
        changed += 1
    if not changed:
        raise ValueError('no expected HGM strategies')
    args.output.mkdir(parents=True, exist_ok=True)
    geometry.write(args.output / 'display_manager_config.xml', encoding='utf-8', xml_declaration=True)
    # ElementTree omits comments before the root, including the source license.
    source_policy = (args.donor / 'hgm_policy_config.xml').read_text()
    preamble = source_policy[:source_policy.index('<HgmConfig')]
    (args.output / 'hgm_policy_config.xml').write_text(
        preamble + ET.tostring(policy.getroot(), encoding='unicode') + '\n')
    (args.output / 'manifest.json').write_text(json.dumps({
        'capture_sha256': hashlib.sha256(args.capture.read_bytes()).hexdigest(),
        'native_width': width, 'native_height': height, 'dpi_from_capture': density,
        'native_refresh_rates': rates, 'native_cutout': [left, 0, right-left, bottom],
        'refresh_strategies_updated': changed, 'hardware_tested': False,
        'scope': 'Native mode geometry; captured Android density retained as initial UI policy'
    }, indent=2) + '\n')
    print(f'UI overlay: {width}x{height}, dpi={density}, native rates={rates}, cutout={left},0,{right-left},{bottom}')


if __name__ == '__main__':
    main()
