#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
from pathlib import Path
import hashlib,json,re,subprocess
root=Path(__file__).resolve().parents[2]
b=(root/'work/vendor-boot-stock/ramdisk.raw.cpio').read_bytes()
out=root/'work/stock-early-modules';out.mkdir(exist_ok=True)
pos=0
while pos<len(b):
 if b[pos:pos+1]==b'\0':
  pos+=1;continue
 if b[pos:pos+6] not in (b'070701',b'070702'): raise ValueError(f'bad cpio header {pos}')
 fields=[int(b[pos+6+i*8:pos+14+i*8],16) for i in range(13)]
 mode,size,nlen=fields[1],fields[6],fields[11]
 if not 0<nlen<4096: raise ValueError('bad cpio name length')
 name=b[pos+110:pos+110+nlen-1].decode()
 start=(pos+110+nlen+3)&~3
 if start+size>len(b): raise ValueError('truncated cpio')
 if mode & 0o170000 == 0o100000 and name.startswith('lib/modules/') and name.endswith('.ko'):
  (out/Path(name).name).write_bytes(b[start:start+size])
 pos=(start+size+3)&~3
paths={}
conflicts=[]
for directory in (root/'work/stock-modules',out):
 for p in directory.glob('*.ko'):
  info=subprocess.check_output(['modinfo',str(p)],text=True)
  vals={}
  for line in info.splitlines():
   if ':' in line:
    k,v=line.split(':',1);vals.setdefault(k,[]).append(v.strip())
  name=vals['name'][0].replace('-','_')
  digest=hashlib.sha256(p.read_bytes()).hexdigest()
  if name in paths and paths[name]['sha256']!=digest:conflicts.append(name)
  paths[name]={'path':str(p.relative_to(root)),'sha256':digest,'vermagic':vals['vermagic'][0],
    'depends':[s.replace('-','_') for s in vals.get('depends',[''])[0].split(',') if s],
    'softdep':vals.get('softdep',[])}
order=[];done=set();active=set();missing=set();missing_soft=set()
def visit(name):
 if name in done:return
 if name not in paths:missing.add(name);return
 if name in active:raise ValueError('dependency cycle '+name)
 active.add(name)
 entry=paths[name]
 deps=list(entry['depends'])
 for soft in entry['softdep']:
  pre=soft.partition('pre:')[2].partition('post:')[0]
  for softname in pre.split():
   normalized=softname.replace('-','_')
   if normalized in paths:deps.append(normalized)
   else:missing_soft.add(normalized)
 for d in deps:visit(d)
 active.remove(name);done.add(name);order.append(name)
for name in ('msm_drm','msm_kgsl'):visit(name)
r={'scope':'Offline hard/pre-soft dependency inventory only; no modules loaded; symbol CRCs and signatures not validated',
   'early_modules':len(list(out.glob('*.ko'))),'all_modules':len(paths),'conflicting_module_bytes':conflicts,
   'unresolved_hard_dependencies':sorted(missing),'unresolved_soft_pre_hints':sorted(missing_soft),'closure_order':order,'closure':[dict(name=n,**paths[n]) for n in order]}
p=root/'out/stock-dsu-audit/graphics-modules.json';p.write_text(json.dumps(r,indent=2)+'\n')
print({k:v for k,v in r.items() if k not in ('closure','closure_order')})
print('closure modules',len(order))
