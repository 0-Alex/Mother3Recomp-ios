import sys
ev=[]
def tap(f,k,d=4): ev.append((f,k)); ev.append((f+d,0)); return f+d+10
for f in range(200,900,90): tap(f,1)
f=950
for n in range(12):
    for i in range(6): f=tap(f,128)
    f=tap(f,1)+20
    f=tap(f,8)+10
    f=tap(f,1)+60
for k in (128,128,1,1): f=tap(f,k)+30
# then confirmations / dialogue: A every 40 frames
cyc=[128,128,1,1,1]
k=0
while f<int(sys.argv[2]): f=tap(f,cyc[k%5])+36; k+=1
import random
random.seed(int(sys.argv[3]) if len(sys.argv)>3 else 1)
end=int(sys.argv[4]) if len(sys.argv)>4 else f
while f<end:
    d=random.choice([16,32,64,128,16|64,32|128,1,2,256,512,8])
    hold=random.randint(10,90); ev.append((f,d)); ev.append((f+hold,0)); f+=hold+5
open(sys.argv[1],'w').write('\n'.join(f'{a} {b}' for a,b in sorted(ev)))
