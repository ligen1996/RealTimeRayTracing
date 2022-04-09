import os
import re
import shutil
import skimage
os.environ["OPENCV_IO_ENABLE_OPENEXR"] = "1"

import cv2

gExp = None
gExpNames = ["CompareAll", "Banding", "SMV"]

useRelease = True
gComparerExe = "../../Bin/x64/%s/ImageCompare.exe" % ("Release" if useRelease else "Debug")

def getRMSE(vFileName1, vFileName2, vOutHeatMapFileName = None):
    cmd = "start /wait /b %s \"%s\" \"%s\" -m rmse" % (gComparerExe, vFileName1, vFileName2)
    if vOutHeatMapFileName:
        cmd += " -e \"%s\"" % vOutHeatMapFileName
    res = os.popen(cmd).read()
    return float(res)

def getSSIM(vFileName1, vFileName2):
    img1 = cv2.imread(vFileName1, cv2.IMREAD_UNCHANGED)
    img2 = cv2.imread(vFileName2, cv2.IMREAD_UNCHANGED)

    SSIM = 0.0
    for channel in range(3):
        SSIM += skimage.metrics.structural_similarity(img1[:, :, channel], img2[:, :, channel])
    return SSIM / 3


BaseDir = "E:/Out/Irregular/"

def getFileName(Scene, Alg, Shape):
    return BaseDir + "%s-%s-%s.png" % (Scene, Alg, Shape)

Text = ''
def addEntry(Scene, Shape, RMSE, SSIM):
    global Text
    Text += "Scene: %s, Shape: %s\n" % (Scene, Shape)
    Text += "  RMSE: %s\n" % RMSE
    Text += "  SSIM: %s\n" % SSIM

def writeToFile():
    FileRMSE = open(BaseDir + "Measure.txt", "w")
    FileRMSE.write(Text)
    FileRMSE.close()

AlgGT = 'GroundTruth'
AlgTarget = 'SRGM'
for Scene in ['Grid', 'Dragon', 'Robot']:
    for Shape in ['rect', 'star', '2']:
        OutHeatMapFileName = BaseDir + "%s_%s_heatmap.png" % (Scene, Shape)
        GTFile = getFileName(Scene, AlgGT, Shape)
        TargetFile = getFileName(Scene, AlgTarget, Shape)
        RMSE = getRMSE(GTFile, TargetFile, OutHeatMapFileName)
        SSIM = getSSIM(GTFile, TargetFile)
        addEntry(Scene, Shape, RMSE, SSIM)
writeToFile()
os.system("pause")