#!/usr/bin/env python3
import cv2 as cv
import sys
import numpy as np
from time import sleep

if len(sys.argv) < 2:
    print("filename is required")
    exit(2)

out=open(sys.argv[1],'rb')
fmt=int.from_bytes(out.read(4), 'little')
width=int.from_bytes(out.read(4), 'little')
height=int.from_bytes(out.read(4), 'little')
size = width * height * 3

#if fmt != 1:
#    print("error: unsupported format")
#    exit(1)

while(True):
    buf=out.read(size)
    frame = np.frombuffer(buffer=buf, dtype=np.uint8).reshape((height, width, 3))
    cv.imshow('frame', frame)
    if cv.waitKey(0) & 0xFF == ord('q'):
        break
    sleep(0.03)
