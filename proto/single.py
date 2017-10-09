#!/usr/bin/env python3
import cv2 as cv
import numpy as np

cap = cv.VideoCapture(0)
fourcc = cv.VideoWriter_fourcc(*'YUYV')
cap.set(6, fourcc);


out=open('/tmp/dataset.raw','wb')

# write header
hdr={'fmt': 6, 'width': 640, 'height': 480}
for i in hdr:
    out.write(hdr[i].to_bytes(4,"little"))

if not cap.isOpened():
    print("cannot open camera")
    exit(1)


#while True:
ret, frame = cap.read()

    # Display the resulting frame
frame = cv.flip(frame, 1)
out.write(frame.tobytes())
cv.imshow('frame', frame)

#    if cv.waitKey(1) & 0xFF == ord('q'):
#   break

# When everything done, release the capture
cap.release()
cv.destroyAllWindows()
