#!/usr/bin/env python
fd=open('frame.txt', 'w')
for i in range(1024):
    r='f' if i==0 else '0'
    g='f' if i==1023 else '0'
    b=str(i%10)
    fd.write(r+g+b)
    if i < 1023:
        fd.write('\n')
fd.close()
