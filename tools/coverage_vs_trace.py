#!/usr/bin/env python3
"""Compare generated dispatch coverage with PCs executed by the mGBA reference tracer.
usage: coverage_vs_trace.py generated/dispatch_table.cpp hits.bin [hits2.bin ...]"""
import re, sys, numpy as np, collections
tab=set()
for m in re.finditer(r'\{0x([0-9A-F]{8})u, (\d)u, \du,', open(sys.argv[1]).read()):
    tab.add((int(m.group(1),16), int(m.group(2))))
acc=None
for f in sys.argv[2:]:
    h=np.frombuffer(open(f,'rb').read(),np.uint8)
    acc=h.copy() if acc is None else acc|h
segs=((0,0x1000000,0x08000000),(0x1000000,0x1004000,0x03000000),(0x1004000,len(acc),0x02000000))
miss=[]; total=0
for a,b,base in segs:
    h=acc[a:b]
    for i in np.nonzero(h)[0]:
        pc=base+int(i)*2; v=int(h[i])
        for bit,thumb in ((1,1),(2,0)):
            if v&bit:
                total+=1
                if (pc,thumb) not in tab: miss.append((pc,thumb))
print(f'executed (pc,mode) pairs: {total}; missing from static dispatch: {len(miss)}')
c=collections.Counter((p>>8)<<8 for p,t in miss)
for k,v in sorted(c.items())[:80]: print(hex(k),v)
open('/tmp/missing_pcs.txt','w').write('\n'.join(f'{p:08X} {"thumb" if t else "arm"}' for p,t in sorted(miss)))
