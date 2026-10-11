#!/usr/bin/env python3
"""Convert 'frame activehighmask' scripts to gbarecomp-keyinput-v1 replay files."""
import sys
print('# gbarecomp-keyinput-v1')
print('0,0x03FF')
for line in open(sys.argv[1]):
    f,k=line.split(); print(f'{int(f)},0x{0x3FF ^ int(k,0):04X}')
