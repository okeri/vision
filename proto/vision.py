#!/usr/bin/env python3
import cv2 as cv
import numpy as np
import sys
import time
if len(sys.argv) < 2:
    print("filename is required")
    exit(2)

cap=open(sys.argv[1],'rb')

fmt=int.from_bytes(cap.read(4), 'little')
width=int.from_bytes(cap.read(4), 'little')
height=int.from_bytes(cap.read(4), 'little')
size = width * height * 3

if fmt != 6:
    print("error: unsupported format")
    exit(1)


orb = cv.ORB_create()
bf = cv.BFMatcher(cv.NORM_HAMMING, crossCheck=True)

stats = []
frames = []
frames.append({"data": None, "feature": None, "handle": None})
frames.append({"data": None, "feature": None, "handle": None})
frameToggler = False
m = False


while True:
    buf=cap.read(size)
    if (len(buf) == 0):
        break
    image = np.frombuffer(buffer=buf, dtype=np.uint8).reshape((height, width, 3))
    frames[frameToggler]["data"] = cv.cvtColor(image, cv.COLOR_BGR2GRAY)
    cv.medianBlur(frames[frameToggler]["data"], 3, frames[frameToggler]["data"])

    if m == True:
        frames[frameToggler]["feature"], frames[frameToggler]["handle"] = orb.detectAndCompute(frames[frameToggler]["data"], None)
    frames[not frameToggler]["feature"], frames[not frameToggler]["handle"] = orb.detectAndCompute(frames[not frameToggler]["data"], None)
    result=0
    #   result=cv.drawKeypoints(frames[frameToggler]["data"],frames[frameToggler]["feature"],result,color=(255,0,0),flags=0)
    matches = bf.match(frames[frameToggler]["handle"], frames[not frameToggler]["handle"])
    matches = sorted(matches, key = lambda x:x.distance)
    tmatches = [s for s in matches if s.distance < 20]
    stats.append(len(tmatches))
    result = cv.drawMatches(frames[frameToggler]["data"],frames[frameToggler]["feature"],frames[not frameToggler]["data"],frames[not frameToggler]["feature"], tmatches, None, flags=2)

    # Display the resulting frame
    cv.imshow('frame', result)
    if cv.waitKey(0) & 0xFF == ord('q'):
        break
    m = True
    frameToggler = not frameToggler

# print result
print("total: {}, min: {}, max: {}, avg: {}".format(len(stats), min(stats), max(stats), np.mean(stats)))
# When everything done, release the capture
cv.destroyAllWindows()
