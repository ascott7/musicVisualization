#!/usr/bin/env python

import sys

fname=""
type=""
try:
    fname=sys.argv[1]
    type=sys.argv[2]
except e:
    print "usage: make_frame fname type"
    exit(1)
fd=open(fname, 'w')
for i in range(1024):
    if type == 'p': # patern
        r='f' if i==0 else '0'
        g='f' if i==1023 else '0'
        b=str(i%10)
        fd.write(r+g+b)
    elif  type == '1': # all 1's
        fd.write('fff')
    elif type == '0': # all 0's
        fd.write('000')
    elif type == 'r': # each row is different
        fd.write(3*str((i/32)%10))
    if i < 1023:
        fd.write('\n')
fd.close()
