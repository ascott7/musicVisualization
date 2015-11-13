#!/usr/bin/env python

import sys

sys.stdout.write('{')

written=0
for i in range(256):
    rev=0
    for j in range(8):
        rev |= ((i >> j) & 1) << (7-j)
    str_rev="{0:#0{1}x}".format(rev,4)
    written+=len(str_rev)
    if (written > 60):
        written%=60
        sys.stdout.write('\n ')
    sys.stdout.write(str_rev)
    if (i < 255):
        sys.stdout.write(', ')
        written+=2

sys.stdout.write('}\n')
