#!/usr/bin/env python
fd=open('frame.txt', 'w')
for i in range(1024):
    fd.write('00f')
    if i < 1023:
        fd.write('\n')
fd.close()
