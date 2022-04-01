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

def copyOutput(vSearchBaseDir, vFrameId, vPass, vPort, vOutputDir, vNewName):
    TargetImage = findOutput(vSearchBaseDir, vFrameId, vPass, vPort)
    if not TargetImage:
        raise
    Ext = getExtension(TargetImage)
    shutil.copyfile(TargetImage, vOutputDir + vNewName + Ext) 

def run(vBaseDir, vDirGT, vDirTargets, vOutputDir = None):
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


    OutputBaseDir = vOutputDir if vOutputDir else vBaseDir
    if not os.path.exists(OutputBaseDir):
        os.makedirs(OutputBaseDir)
    # find min
    NameT1 = vDirTargets[0]['Name'] # name of test 1 (SMV)
    NameT2 = vDirTargets[1]['Name'] # name of test 2 (NOSMV)

    def generateCompareResultString(Result):
        String = "  Group %d, Frame %d [ %s ]\n" % (Result['Group'], Result['FrameId'], Result['Image'])
        for Measure in Measures:
            MeasureName = Measure['Name']
            Measure_T1 = Result['TargetResult'][0][MeasureName]
            Measure_T2 = Result['TargetResult'][1][MeasureName]
            String = String + "  %s %s: %.8f\n" % (MeasureName, NameT1, Measure_T1)
            String = String + "  %s %s: %.8f\n" % (MeasureName, NameT2, Measure_T2)
            String = String + "  (%s - %s): %.8f\n" % (NameT2, NameT1, Measure_T2 - Measure_T1)
        return String

    def printCompareResult(Result):
        print(generateCompareResultString(Result))

    def copyCompareResult(Result, Name):
        OutputDir = OutputBaseDir + Name + "/"
        if not os.path.exists(OutputDir):
            os.makedirs(OutputDir)
        Ext = getExtension(Result['Image'])
        GTImageName = vBaseDir + vDirGT + Result['Image']
        shutil.copyfile(GTImageName, OutputDir + 'GroundTruth' + Ext)
        copyOutput(vBaseDir + "/GroundTruth", FrameId, "AccumulatePass", "output", OutputDir, "GroundTruth_vis") # visibility
        for i, Target in enumerate(vDirTargets):
            TargetBaseDir = vBaseDir + Target['Name']
            TargetCompareDir = vBaseDir + Target['Dir']
            TargetResult = Result['TargetResult'][i]
            Ext = getExtension(TargetResult['Image'])
            TargetImageName = TargetCompareDir + TargetResult['Image']
            shutil.copyfile(TargetImageName, OutputDir + Target['Name'] + Ext)
            
            copyOutput(TargetBaseDir, FrameId, "STSM_BilateralFilter", "Result", OutputDir, Target['Name'] + "_vis") # visibility
            if (gExpName == 'Banding'): # not filtered image for banding exp
                copyOutput(TargetBaseDir, FrameId, "STSM_TemporalReuse", "TR_Visibility", OutputDir, Target['Name'] + "_original_vis") # original visibility
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

    def getMeans(vMeasureName):
        MeanT1 = 0.0
        MeanT2 = 0.0
        MeanDelta= 0.0
        for i, Result in enumerate(CompareResult):
            M_T1 = Result['TargetResult'][0][vMeasureName]
            M_T2 = Result['TargetResult'][1][vMeasureName]
            MeanT1 += M_T1
            MeanT2 += M_T2
            MeanDelta += M_T2 - M_T1
        MeanT1 = MeanT1 / len(CompareResult)  
        MeanT2 = MeanT2 / len(CompareResult)  
        MeanDelta = MeanDelta / len(CompareResult)  
        return [MeanT1, MeanT2, MeanDelta]
    
    MeanString = ''
    T1_Name = vDirTargets[0]['Name']
    T2_Name = vDirTargets[1]['Name']
    for Measure in Measures:
        MeasureName = Measure['Name']
        [MeanT1, MeanT2, MeanDelta] = getMeans(MeasureName)
        MeanString = MeanString + MeasureName + "\n"
        MeanString = MeanString + "    %s Mean = %.8f\n" % (T1_Name, MeanT1)
        MeanString = MeanString + "    %s Mean = %.8f\n" % (T2_Name, MeanT2)
        MeanString = MeanString + "    (%s - %s) Mean = %.8f\n" % (T2_Name, T1_Name, MeanDelta)
    File = open(OutputBaseDir + "/mean.txt", "w")
    File.write(MeanString)
    File.close()

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
gExpName = gExpNames[gExp - 1]
# if (gExpName == "Ghosting"):
#     # Ghosting 
#     BaseDir = "D:/Out/Ghosting/"
#     DirGT = "GroundTruth/LTCLight-Color/"
#     DirTarget = [
#         {
#             'Name': 'SRGM',
#             'Dir': "SRGM/LTCLight-Color/"
#         },
#         {
#             'Name': 'TA',
#             'Dir': "TA/LTCLight-Color/"
#         },
#     ]
#     for Scene in ['Grid', 'Dragon', 'Robot']:
#         for Type in ['Object', 'Light']:
#             SubDir = Scene + "/" + Type + "/"
#             run(BaseDir + SubDir, DirGT, DirTarget)

if gExpName == "Banding":
    # Banding Compare Self / Tranditional 
    BaseDir = "D:/Out/Banding/"
    DirGT = "GroundTruth/LTCLight-Color/"
    DirTarget = []
    OutputBaseDir = BaseDir
    
    def runTarget(vDirTarget, vOutputBaseDir):
        for Scene in ['GridObserve', 'DragonObserve', 'RobotObserve']:
            SubDir = Scene + "/"
            run(BaseDir + SubDir, DirGT, vDirTarget, vOutputBaseDir + SubDir)

    # compare self
    DirTarget = [
        {
            'Name': 'SRGM_Random',
            'Dir': "SRGM_Random/LTCLight-Color/"
        },
        {
            'Name': 'SRGM_Banding',
            'Dir': "SRGM_Banding/LTCLight-Color/"
        },
    ]
    OutputBaseDir = BaseDir + "Banding_CompareSelf/"
    runTarget(DirTarget, OutputBaseDir)
    # compare tranditional
    DirTarget = [
        {
            'Name': 'SRGM_Random',
            'Dir': "SRGM_Random/LTCLight-Color/"
        },
        {
            'Name': 'Tranditional',
            'Dir': "Tranditional/LTCLight-Color/"
        },
    ]
    OutputBaseDir = BaseDir + "Banding_CompareTranditional/"
    runTarget(DirTarget, OutputBaseDir)
elif gExpName == 'CompareAll':
    BaseDir = "D:/Out/CompareAll/"
    DirGT = "GroundTruth/LTCLight-Color/"
    DirTarget = []
    if (True): # compare self
        DirTarget = [
            {
                'Name': 'SRGM_Adaptive',
                'Dir': "SRGM_Adaptive/LTCLight-Color/"
            },
            {
                'Name': 'SRGM_NoAdaptive',
                'Dir': "SRGM_NoAdaptive/LTCLight-Color/"
            },
        ]
    else: # compare tranditional
        DirTarget = [
            {
                'Name': 'SRGM_Adaptive',
                'Dir': "SRGM_Adaptive/LTCLight-Color/"
            },
            {
                'Name': 'Tranditional',
                'Dir': "Tranditional/LTCLight-Color/"
            },
        ]
    for Scene in ['Grid', 'Dragon', 'Robot']:
        for Type in ['Object', 'Light']:
            SubDir = Scene + "/" + Type + "/"
            run(BaseDir + SubDir, DirGT, DirTarget)
elif gExpName == 'SMV':
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