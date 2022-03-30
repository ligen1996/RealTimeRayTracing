import os
import re
import shutil
import skimage
os.environ["OPENCV_IO_ENABLE_OPENEXR"] = "1"

import cv2

gExp = None
gExpNames = ["Ghosting", "BandingCompareSelf", "BandingCompareTranditional", "SMV"]

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

def getExtension(vFileName):
        return os.path.splitext(vFileName)[-1]

def extractFrameId(vFileName):
    Match = re.search(r".*\.(\d+)\.(exr|png)", vFileName)
    if (Match == None):
        return None
    FrameId = int(Match.group(1))
    return FrameId

def findSameFrame(vDir, vFrameId):
    FileNames = os.listdir(vDir)
    for FileName in FileNames:
        if (os.path.isdir(FileName)):
            continue
        Match = re.search(r".*\." + str(vFrameId) + r"\.(exr|png)", FileName)
        if Match:
            return FileName
    return None

# vBaseDir is the base folder of an exp of one scene and one graph
def findOutput(vBaseDir, vFrameId, vPass, vPort):
    FileNames = os.listdir(vBaseDir)
    for FileName in FileNames:
        if not os.path.isdir(vBaseDir + "/" + FileName):
            continue
        if FileName == (vPass + "-" + vPort):
            Dir = vBaseDir + "/" + FileName + "/"
            File = findSameFrame(Dir, vFrameId)
            if File:
                return Dir + File
            else:
                return None
    return None

def run(vBaseDir, vDirGT, vDirTargets):
    CompareResult = []
    GTList = os.listdir(vBaseDir + vDirGT)
    for i, GTImageName in enumerate(GTList):
        FrameId = extractFrameId(GTImageName)
        if not FrameId:
            print("Can parse file name ", GTImageName)
            continue

        Result = {}
        Result['Group'] = i + 1
        Result['FrameId'] = FrameId
        Result['Image'] = GTImageName
        Result['TargetResult'] = []
        
        for Target in vDirTargets:
            TargetImageName = findSameFrame(vBaseDir + Target['Dir'], FrameId)
            if not TargetImageName:
                print("No match found for file ", GTImageName)
                continue
            
            GTFile = vBaseDir + vDirGT + GTImageName
            TargetFile = vBaseDir + Target['Dir'] + TargetImageName

            TargetResult = {}
            TargetResult['Name'] = Target['Name']
            TargetResult['Image'] = TargetImageName

            for Measure in Measures:
                TargetResult[Measure['Name']] = Measure['Func'](GTFile, TargetFile)
            
            Result['TargetResult'].append(TargetResult)
        CompareResult.append(Result)

    # print all
    for i, Result in enumerate(CompareResult):
        print("Group %d, Frame %d: [ %s ]" % (Result['Group'], Result['FrameId'], Result['Image']))
        for TargetResult in Result['TargetResult']:
            for Measure in Measures:
                print("  Target %s, RMSE = %.8f [ %s ]" % (TargetResult['Name'], TargetResult[Measure['Name']], TargetResult['Image']))


    gOutputBaseDir = vBaseDir + "best/"
    # find min
    NameT1 = vDirTargets[0]['Name'] # name of test 1 (SMV)
    NameT2 = vDirTargets[1]['Name'] # name of test 2 (NOSMV)

    def generateCompareResultString(Result):
        String = "  Group %d, Frame %d [ %s ]\n" % (Result['Group'], Result['FrameId'], Result['Image'])
        for Measure in Measures:
            MeasureName = Measure['Name']
            Measure_T1 = Result['TargetResult'][0][MeasureNameMeasureName]
            Measure_T2 = Result['TargetResult'][1][MeasureNameMeasureName]
            String = String + "  %s %s: %.8f\n" % (MeasureName, NameT1, Measure_T1)
            String = String + "  %s %s: %.8f\n" % (MeasureName, NameT2, Measure_T2)
            String = String + "  (%s - %s): %.8f\n" % (NameT2, NameT1, Measure_T2 - Measure_T1)

    def printCompareResult(Result):
        print(generateCompareResultString(Result))

    def copyCompareResult(Result, Name):
        OutputDir = gOutputBaseDir + Name + "/"
        if not os.path.exists(OutputDir):
            os.makedirs(OutputDir)
        Ext = getExtension(Result['Image'])
        GTImageName = vBaseDir + vDirGT + Result['Image']
        shutil.copyfile(GTImageName, OutputDir + 'GroundTruth' + Ext)
        TargetShadedImage = findOutput(vBaseDir + "/GroundTruth", FrameId, "AccumulatePass", "output")
        if not TargetShadedImage:
                raise
        shutil.copyfile(TargetShadedImage, OutputDir + "GroundTruth_vis" + Ext) 
        for i, Target in enumerate(vDirTargets):
            TargetBaseDir = vBaseDir + Target['Name']
            TargetCompareDir = vBaseDir + Target['Dir']
            TargetResult = Result['TargetResult'][i]
            Ext = getExtension(TargetResult['Image'])
            TargetImageName = TargetCompareDir + TargetResult['Image']
            shutil.copyfile(TargetImageName, OutputDir + Target['Name'] + Ext)
            TargetShadedImage = findOutput(TargetBaseDir, FrameId, "STSM_BilateralFilter", "Result")
            if not TargetShadedImage:
                raise
            shutil.copyfile(TargetShadedImage, OutputDir + Target['Name'] + "_vis" + Ext) 

            if (gExp == 2 or gExp == 3): # not filtered image for banding exp
                TargetOriginalImage = findOutput(TargetBaseDir, FrameId, "STSM_TemporalReuse", "TR_Visibility")
                if not TargetOriginalImage:
                    raise
                NewImage = OutputDir + Target['Name'] + "_original" + Ext
                shutil.copyfile(TargetOriginalImage, OutputDir + Target['Name'] + "_original" + Ext) 
            OutHeatMapFileName = OutputDir + Target['Name'] + "_heatmap.png"
            getRMSE(GTImageName, TargetImageName, OutHeatMapFileName)

        CompareResultString = generateCompareResultString(Result)
        FileRMSE = open(OutputDir + "Measure.txt", "w")
        FileRMSE.write(CompareResultString)
        FileRMSE.close()

    def findBest(vMeasureName, preferMin = True):
        # min RMSE/SSIM
        BestIndex = -1
        BestValue = float("inf") if preferMin else float("-inf") 
        for i, Result in enumerate(CompareResult):
            M = Result['TargetResult'][0][vMeasureName]
            if preferMin:
                if (BestValue > M):
                    BestIndex = i
                    BestValue = M
            else:
                if (BestValue < M):
                    BestIndex = i
                    BestValue = M
                
        print("Best %s:" % vMeasureName)
        printCompareResult(CompareResult[BestIndex])
        copyCompareResult(CompareResult[BestIndex], "Best %s" % vMeasureName)

    findBest("RMSE", True)
    findBest("SSIM", False)

    # max relative RMSE
    def findRelativeBest(vMeasureName, preferMin = True):
        BestIndex = -1
        BestRelativeValue = float("inf") if preferMin else float("-inf") 
        for i, Result in enumerate(CompareResult):
            M_T1 = Result['TargetResult'][0][vMeasureName]
            M_T2 = Result['TargetResult'][1][vMeasureName]
            d = M_T2 - M_T1
            if preferMin:
                if (BestRelativeValue > d):
                    BestIndex = i
                    BestRelativeValue = d
            else:
                if (BestRelativeValue < d):
                    BestIndex = i
                    BestRelativeValue = d
        print("Best relative %s:" % vMeasureName)
        printCompareResult(CompareResult[BestIndex])
        copyCompareResult(CompareResult[BestIndex], "Best relative %s" % vMeasureName)

    findRelativeBest("RMSE", False)
    findRelativeBest("SSIM", True)

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

gExp = chooseExp()
if (gExp == 1):
    # Ghosting 
    BaseDir = "D:/Out/Ghosting/"
    DirGT = "GroundTruth/LTCLight-Color/"
    DirTarget = [
        {
            'Name': 'SRGM',
            'Dir': "SRGM/LTCLight-Color/"
        },
        {
            'Name': 'TA',
            'Dir': "TA/LTCLight-Color/"
        },
    ]
    for Scene in ['Grid', 'Dragon', 'Robot']:
        for Type in ['Object', 'Light']:
            SubDir = Scene + "/" + Type + "/"
            run(BaseDir + SubDir, DirGT, DirTarget)

elif gExp == 2 or gExp == 3:
    # Banding Compare Self / Tranditional 
    BaseDir = "D:/Out/" + ("BandingCompareSelf/" if gExp == 2 else "BandingCompareTranditional/")
    DirGT = "GroundTruth/LTCLight-Color/"
    if gExp == 2:
        DirTarget = [
            {
                'Name': 'Random',
                'Dir': "Random/LTCLight-Color/"
            },
            {
                'Name': 'Banding',
                'Dir': "Banding/LTCLight-Color/"
            },
        ]
    else:
        DirTarget = [
            {
                'Name': 'Random',
                'Dir': "Random/LTCLight-Color/"
            },
            {
                'Name': 'Tranditional_16',
                'Dir': "Tranditional_16/LTCLight-Color/"
            },
        ]
    for Scene in ['GridObserve', 'DragonObserve', 'ArcadeObserve']:
        SubDir = Scene + "/"
        run(BaseDir + SubDir, DirGT, DirTarget)
elif gExp == 4:
    # SMV 
    BaseDir = "D:/Out/SMV/"
    DirGT = "GroundTruth/LTCLight-Color/"
    DirTarget = [
        {
            'Name': 'SMV',
            'Dir': "SMV/LTCLight-Color/"
        },
        {
            'Name': 'NoSMV',
            'Dir': "NoSMV/LTCLight-Color/"
        },
    ]
    for Scene in ['Grid', 'Dragon', 'Robot']:
        for Type in ['Object', 'Light']:
            SubDir = Scene + "/" + Type + "/"
            run(BaseDir + SubDir, DirGT, DirTarget)
os.system("pause")