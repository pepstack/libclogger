#!/usr/bin/python

import os, time

i = 0

while True:
    time.sleep(0.1)
    os.system("./mklogapp -n1000")
    i += 1
    print i
