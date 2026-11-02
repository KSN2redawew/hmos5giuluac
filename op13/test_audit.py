# SPDX-License-Identifier: Apache-2.0
import json
from pathlib import Path
import tempfile
import unittest
from audit import inventory_configs


class StartupAuditTests(unittest.TestCase):
    def test_reports_physical_mount_and_preserves_full_service_definition(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'init').mkdir()
            service = {'name': 'foundation', 'path': ['/system/bin/sa_main'],
                       'uid': 'foundation', 'permission': ['ohos.permission.MANAGE_SETTINGS']}
            (root / 'init' / 'foundation.cfg').write_text(json.dumps({
                'services': [service], 'jobs': [{'name': 'pre-init', 'cmds': [
                    'mkdir /data/service 0711 root root',
                    'mount ext4 /dev/block/by-name/persist /persist rw']}]}))
            result = inventory_configs(root)
            self.assertEqual(result['services'][0]['definition'], service)
            self.assertEqual(len(result['commands_requiring_review']), 1)
            self.assertIn('block-device', result['commands_requiring_review'][0]['reasons'][0])
            self.assertFalse(result['parse_errors'])

    def test_malformed_startup_is_visible_and_unrelated_cfg_is_excluded(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'init').mkdir()
            (root / 'init' / 'broken.cfg').write_text('{bad json')
            (root / 'init' / 'badjob.cfg').write_text('{"jobs": ["a"]}')
            (root / 'unrelated.cfg').write_text('{"jobs": {"arbitrary": "config"}}')
            result = inventory_configs(root)
            self.assertEqual(len(result['parse_errors']), 2)
            self.assertFalse(result['jobs'])


if __name__ == '__main__':
    unittest.main()
