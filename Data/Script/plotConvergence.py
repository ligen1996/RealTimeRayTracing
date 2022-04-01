import os
import re
import shutil
import skimage
import matplotlib.pyplot as plt
import json
from matplotlib.font_manager import FontProperties
os.environ["OPENCV_IO_ENABLE_OPENEXR"] = "1"

import cv2

gFontYaHei = FontProperties(fname="C:/Windows/Fonts/msyh.ttc", size=14)

gExpNames = ["SMV", "Filter"]

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

Measures = [
    {
        'Name': 'RMSE',
        'Func': getRMSE
    },
    {
        'Name': 'SSIM',
        'Func': getSSIM
    },
]

def extractFrameId(vFileName):
    Match = re.search(r".*\-(\d+)\..*(exr|png)", vFileName)
    if (Match == None):
        return None
    FrameId = int(Match.group(1))
    return FrameId

def findSameFrame(vDir, vFrameId):
    FileNames = os.listdir(vDir)
    for FileName in FileNames:
        if (os.path.isdir(FileName)):
            continue
        Match = re.search(r".*\-0*" + str(vFrameId) + r"\..*(exr|png)", FileName)
        if Match:
            return FileName
    return None

def calConvergence(vBaseDir, vDirGT, vDirTarget):
    CompareResult = []
    GTImages = os.listdir(vBaseDir + vDirGT)

    Num = len(GTImages)
    TargetResults = {}
    for Target in vDirTarget:
        TargetName = Target['Name']
        TargetResults[TargetName] = {}
        TargetResults[TargetName]['RMSE'] = []
        TargetResults[TargetName]['SSIM'] = []

    print("Calculating measure...")
    for i, GTImage in enumerate(GTImages):
        FrameId = extractFrameId(GTImage)
        if FrameId == None:
            print("Can parse file name ", GTImage)
            continue
        
        print("Current frame id:", FrameId)

        for Target in vDirTarget:
            TargetName = Target['Name']
            TargetImage = findSameFrame(vBaseDir + Target['Dir'], FrameId)
            if not TargetImage:
                print("No match found for file ", GTImage)
                continue
            RMSE = getRMSE(vBaseDir + vDirGT + GTImage, vBaseDir + Target['Dir'] + TargetImage)
            SSIM = getSSIM(vBaseDir + vDirGT + GTImage, vBaseDir + Target['Dir'] + TargetImage)
            print("  ", GTImage, " - ", TargetImage)
            print("  RMSE: ", RMSE, " SSIM", SSIM)

            TargetResults[TargetName]['RMSE'].append(RMSE)
            TargetResults[TargetName]['SSIM'].append(SSIM)
    return [TargetResults, Num]

def calFlicking(vBaseDir, vDirGT, vDirTarget):
    CompareResult = []
    GTImages = os.listdir(vBaseDir + vDirGT)

    Num = len(GTImages)
    TargetResults = {}
    for Target in vDirTarget:
        TargetName = Target['Name']
        TargetResults[TargetName] = {}
        TargetResults[TargetName]['RMSE'] = []
        TargetResults[TargetName]['SSIM'] = []

    print("Calculating measure...")
    for i in range(Num - 1):
        FrameId = extractFrameId(GTImages[i])
        if FrameId == None:
            print("Can parse file name ", GTImage)
            continue
        
        print("Current frame id:", FrameId)

        for Target in vDirTarget:
            TargetName = Target['Name']
            TargetImageCur = findSameFrame(vBaseDir + Target['Dir'], FrameId)
            TargetImageNext = findSameFrame(vBaseDir + Target['Dir'], FrameId + 1)
            if not TargetImageCur or not TargetImageNext:
                print("No match found for file ", GTImage)
                continue
            RMSE = getRMSE(vBaseDir + Target['Dir'] + TargetImageCur, vBaseDir + Target['Dir'] + TargetImageNext)
            SSIM = getSSIM(vBaseDir + Target['Dir'] + TargetImageCur, vBaseDir + Target['Dir'] + TargetImageNext)
            print("  ", TargetImageCur, " - ", TargetImageNext)
            print("  RMSE: ", RMSE, " SSIM", SSIM)

            TargetResults[TargetName]['RMSE'].append(RMSE)
            TargetResults[TargetName]['SSIM'].append(SSIM)
    return [TargetResults, Num - 1]

def plot(vResult, vNum, vPrefix, vReverseLegendPos = False, vSave = False, vSaveDir = ""):
    for Type, Style in [['RMSE', "-"], ['SSIM', '--']]:
        fig = plt.figure(figsize=(12, 9))
        plt.xlabel("Frame", fontsize = 14)
        plt.ylabel(Type, fontsize = 14)

        xData = range(0, vNum)
        Colors = ['#1f77b4', '#ff7f0e', '#2ca02c', '#d62728', '#9467bd', '#8c564b', '#e377c2', '#7f7f7f', '#bcbd22', '#17becf']
        for i, Target in enumerate(vResult):
            Color = Colors[i]
            yData = vResult[Target][Type]
            plt.plot(xData, yData, label=Target, color=Color, linestyle=Style)

        LegendPos = ''
        Range = list(plt.axis())
        if Type == 'RMSE':
            Range[2] = 0.0
            LegendPos = 'upper center' if vReverseLegendPos else 'lower center'
        else:
            Range[3] = 1.0
            LegendPos = 'lower center' if vReverseLegendPos else 'upper center'
        
        plt.legend(loc = LegendPos, prop = gFontYaHei)
        plt.margins(x = 0.01, y = 0.2)
        
        plt.axis(Range)
        if (vSave):
            if not os.path.exists(vSaveDir):
                os.makedirs(vSaveDir)
            plt.savefig("%s/%s.png" % (vSaveDir, vPrefix + "_" + Type))
        plt.show()

def writeJson(vFileName, vData):
    File = open(vFileName, "w")
    File.write(json.dumps(vData))
    File.close()

def readJson(vFileName):
    File = open(vFileName, "r")
    return json.loads(File.read())

def getOutputFile(vBaseDir, vScene, vType):
    return vBaseDir + "plotData_%s_%s.json" % (vType, vScene)

def chooseExp():
    expNum = len(gExpNames)
    def inputExp():
        print("选择要运行的实验：")
        for i in range(expNum):
            print("  %d: %s" % (i + 1, gExpNames[i]))
        print("  0: 退出")
        return input()  
    exp = inputExp()
    while (not exp.isdigit() or int(exp) < 0 or int(exp) > expNum):
        print("输入错误！\n")
        exp = inputExp()
    if exp == 0:
        exit()
    else:
        return int(exp)

gReadFromFile = False

ExpName = gExpNames[chooseExp() - 1]
if ExpName == "SMV":
    BaseDir = "D:/Out/SMV/"
    DirGT = "GroundTruth/LTCLight-Color/"
    DirTargets = [
        {
            'Name': '阴影重投影方法',
            'Dir': "SMV/LTCLight-Color/"
        },
        {
            'Name': '传统重投影方法',
            'Dir': "NOSMV/LTCLight-Color/"
        },
    ]
    for Object in ['Grid', 'Dragon', 'Robot']:
        for MoveType in ['Object', 'Light']:
            WorkDir = BaseDir + Object + "/" + MoveType + "/"
            OutputFile = getOutputFile(BaseDir, Object + "_" + MoveType, "Convergence")
            if gReadFromFile:
                [Result, Num] = readJson(OutputFile)
            else:
                [Result, Num] = calConvergence(WorkDir, DirGT, DirTargets)
                writeJson(OutputFile, [Result, Num])
            print("Ploting...")
            plot(Result, Num, "SMV_" + Object + "_" + MoveType, False, True, BaseDir)

elif ExpName == "Filter":
    BaseDir = "D:/Out/Filter/"
    DirGT = "GroundTruth/LTCLight-Color/"
    DirTargets = [
        {
            'Name': '使用本文空间复用方法',
            'Dir': "Filtered/LTCLight-Color/"
        },
        {
            'Name': '未使用空间复用方法',
            'Dir': "Original/LTCLight-Color/"
        },
        {
            'Name': '传统方法',
            'Dir': "Tranditional/LTCLight-Color/"
        },
    ]
    for Scene in ['GridObserve', 'DragonObserve', 'RobotObserve']: # all
        CalType = "Convergence"
        print("Run plot for ", Scene, CalType)
        SubDir = Scene + "/"
        OutputFile = getOutputFile(BaseDir, Scene, CalType)
        if gReadFromFile:
            [Result, Num] = readJson(OutputFile)
        else:
            [Result, Num] = calConvergence(BaseDir + SubDir, DirGT, DirTargets)
            writeJson(OutputFile, [Result, Num])
        print("Ploting...")
        plot(Result, Num, "Filter_%s_%s" % (CalType, Scene.replace("Observe", "")), True, True, BaseDir)
os.system("pause")