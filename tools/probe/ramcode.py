import numpy as np, sys
hits=np.frombuffer(open(sys.argv[1],'rb').read(),np.uint8)
ram=open(sys.argv[2],'rb').read()
rom=open('/home/claude/work/mother3.gba','rb').read()
iw=hits[0x1000000:0x1004000]; ew=hits[0x1004000:]
def ranges(h,base):
    idx=np.nonzero(h)[0]; out=[]
    if len(idx)==0: return out
    s=idx[0];p=idx[0]
    for i in idx[1:]:
        if i-p>16: out.append((s,p)); s=i
        p=i
    out.append((s,p)); return [(base+a*2,base+b*2+4,set(int(h[x]) for x in range(a,b+1) if h[x])) for a,b in out]
for name,h,base,off in (('IWRAM',iw,0x03000000,0),('EWRAM',ew,0x02000000,0x8000)):
    for a,b,m in ranges(h,base):
        seg=ram[off+(a-base):off+(b-base)]
        pos=rom.find(seg)
        print(name,hex(a),hex(b),hex(b-a),'modes',m,'rom_src',hex(0x08000000+pos) if pos>=0 else None)
