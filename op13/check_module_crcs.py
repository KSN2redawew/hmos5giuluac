#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
from pathlib import Path
import json,subprocess
root=Path(__file__).resolve().parents[2]
p=root/'out/stock-dsu-audit/graphics-modules.json'
r=json.loads(p.read_text())
def query(flag, path):
 result=subprocess.run(['modprobe',flag,str(path)],text=True,capture_output=True)
 if result.returncode:
  section='__ksymtab' if flag=='--show-exports' else '__versions'
  listing=subprocess.check_output(['readelf','-SW',str(path)],text=True)
  if section in listing:raise RuntimeError(result.stderr)
  return []
 return result.stdout.splitlines()
exports={};conflicts=[]
for entry in r['closure']:
 for line in query('--show-exports',root/entry['path']):
  crc,name=line.split()
  if name in exports and exports[name][0]!=crc:conflicts.append(name)
  exports[name]=(crc,entry['name'])
matched=0;unprovided={};mismatches=[]
for entry in r['closure']:
 for line in query('--show-modversions',root/entry['path']):
  crc,name=line.split()
  if name in exports:
   if exports[name][0]==crc:matched+=1
   else:mismatches.append({'module':entry['name'],'symbol':name,'required_crc':crc,'provider':exports[name]})
  else:unprovided[name]=crc
r['crc_checks']={'matching_imports_from_closure_modules':matched,'mismatches':mismatches,
  'conflicting_exports':conflicts,'imports_without_module_provider':len(unprovided),
  'kernel_exports_verified':False,
  'scope':'Only exports from modules in this closure compared; kernel export CRCs, signatures, KMI and runtime behavior remain unverified'}
p.write_text(json.dumps(r,indent=2)+'\n')
print(r['crc_checks'])
