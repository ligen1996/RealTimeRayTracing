import os
import re
import shutil
import skimage
os.environ["OPENCV_IO_ENABLE_OPENEXR"] = "1"

import cv2

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


BaseDir = "E:/Out/NonAverage/"

def getFileName(Scene, Alg, Texture):
    return BaseDir + "%s-%s-%s.png" % (Scene, Alg, Texture)

Text = ''
def addEntry(Scene, Texture, RMSE, SSIM):
    global Text
    Text += "Scene: %s, Texture: %s\n" % (Scene, Texture)
    Text += "  RMSE: %s\n" % RMSE
    Text += "  SSIM: %s\n" % SSIM

def writeToFile():
    FileRMSE = open(BaseDir + "Measure.txt", "w")
    FileRMSE.write(Text)
    FileRMSE.close()

AlgGT = 'GroundTruth'
AlgTarget = 'SRGM'
for Scene in ['Grid', 'Dragon', 'Robot']:
    for Texture in ['Rect', 'GradientQuantified', 'Halo', 'BluredBunny', 'Cell']:
        OutHeatMapFileName = BaseDir + "%s_%s_heatmap.png" % (Scene, Texture)
        GTFile = getFileName(Scene, AlgGT, Texture)
        TargetFile = getFileName(Scene, AlgTarget, Texture)
        RMSE = getRMSE(GTFile, TargetFile, OutHeatMapFileName)
        SSIM = getSSIM(GTFile, TargetFile)
        addEntry(Scene, Texture, RMSE, SSIM)
writeToFile()
os.system("pause")