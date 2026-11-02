#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
from pathlib import Path
import subprocess
root=Path(__file__).resolve().parents[2]
out=root/'work/image-metadata';out.mkdir(exist_ok=True)
masks=[]
for part,target in [('sys_prod','sys_prod'),('chip_prod','chip_prod'),('cust','cust'),('patch','patch_hw')]:
 subprocess.run([str(root/'bin/Linux/x86_64/extract.erofs'),'-i',str(root/'hmos5'/f'{part}.img'),'-x','--only-cfg','-s','-o',str(out)],check=True,stdout=subprocess.DEVNULL)
 cfg=out/'config'/f'{part}_fs_config'
 for line in cfg.read_text().splitlines():
  name=line.split()[0]
  if name.endswith('/etc/init'):
   sub=name.partition('/')[2]
   masks.append('/'+target+'/'+sub)
(root/'out/op13-startup/mask-init-dirs').write_text('\n'.join(sorted(set(masks)))+'\n')
print('Masked automatic init directories:',len(set(masks)))
print('\n'.join(sorted(set(masks))))
