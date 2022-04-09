import sys
import os
os.environ["OPENCV_IO_ENABLE_OPENEXR"] = "1"
import cv2

def getTextureName():
    if len(sys.argv) == 1:
        return input("Texture file name: ")
    else:
        return sys.argv[1]

def guassFilter(img, kernelSize):
    if (kernelSize == 0):
        return img
    return cv2.GaussianBlur(img, (kernelSize, kernelSize), 0)

def getOdd(v):
    v = round(v)
    if v % 2 == 0:
        v = v + 1
    return v

fileName = getTextureName()
img = cv2.imread(fileName, cv2.IMREAD_UNCHANGED)
imgSize = max(img.shape[0], img.shape[1])

# test kernel size
# templateFilterSize = [0, 0, 7.0, 25, 70, 210, 700, 1500, 1800]
# templateImgSize = 780
# print([x * 2 / templateImgSize for x in templateFilterSize])
# kernelSizes = [getOdd(x * 2 * imgSize / templateImgSize) for x in templateFilterSize] # ps, x2 = kernelsize

kernelSizeRatio = [0.0, 0.0, 0.017948717948717947, 0.0641025641025641, 0.1794871794871795, 0.5384615384615384, 1.794871794871795, 3.8461538461538463, 4.615384615384615]
kernelSizes = [getOdd(x * imgSize) for x in kernelSizeRatio]

print(kernelSizes)

for i, kernelSize in enumerate(kernelSizes):
    newFileName = str(i) + ".exr"
    print("Generating ", newFileName, "...")
    newImg = guassFilter(img, kernelSize)
    cv2.imwrite(newFileName, newImg)
