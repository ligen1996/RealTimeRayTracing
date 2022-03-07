import os
import re
import shutil
import skimage

useRelease = True
gComparerExe = "../../Bin/x64/%s/ImageCompare.exe" % ("Release" if useRelease else "Debug")

def getRMSE(vFileName1, vFileName2, vOutHeatMapFileName = None):
    cmd = "start /wait /b %s \"%s\" \"%s\" -m rmse" % (gComparerExe, vFileName1, vFileName2)
    if vOutHeatMapFileName:
        cmd += " -e \"%s\"" % vOutHeatMapFileName
    res = os.popen(cmd).read()
    return float(res)

def getSSIM(vFile1, vFile2):
    img1 = skimage.io.imread(vFile1)
    img2 = skimage.io.imread(vFile2)
    ssim = skimage.metrics.structural_similarity(img1[:, :, 0], img2[:, :, 0])
    return ssim

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

def run(vBaseDir, vDirGT, vDirTarget):
    CompareResult = []
    GTImages = os.listdir(vBaseDir + vDirGT)
    for i, GTImage in enumerate(GTImages):
        FrameId = extractFrameId(GTImage)
        if not FrameId:
            print("Can parse file name ", GTImage)
            continue

        Result = {}
        Result['Group'] = i + 1
        Result['FrameId'] = FrameId
        Result['Image'] = GTImage
        Result['TargetResult'] = []
        
        for Target in vDirTarget:
            TargetImage = findSameFrame(vBaseDir + Target['Dir'], FrameId)
            if not TargetImage:
                print("No match found for file ", GTImage)
                continue
            RMSE = getRMSE(vBaseDir + vDirGT + GTImage, vBaseDir + Target['Dir'] + TargetImage)
            SSIM = getSSIM(vBaseDir + vDirGT + GTImage, vBaseDir + Target['Dir'] + TargetImage)

            TargetResult = {}
            TargetResult['Name'] = Target['Name']
            TargetResult['Image'] = TargetImage
            TargetResult['RMSE'] = RMSE
            TargetResult['SSIM'] = SSIM
            
            Result['TargetResult'].append(TargetResult)
        CompareResult.append(Result)

    # print all
    for i, Result in enumerate(CompareResult):
        print("Group %d, Frame %d: [ %s ]" % (Result['Group'], Result['FrameId'], Result['Image']))
        for TargetResult in Result['TargetResult']:
            print("  Target %s, RMSE = %.8f [ %s ]" % (TargetResult['Name'], TargetResult['RMSE'], TargetResult['Image']))


    gOutputBaseDir = vBaseDir + "best/"
    # find min
    NameT1 = vDirTarget[0]['Name']
    NameT2 = vDirTarget[1]['Name']

    def printCompareInfo(Result):
        RMSE_T1 = Result['TargetResult'][0]['RMSE']
        RMSE_T2 = Result['TargetResult'][1]['RMSE']
        SSIM_T1 = Result['TargetResult'][0]['SSIM']
        SSIM_T2 = Result['TargetResult'][1]['SSIM']
        print("  Group %d, Frame %d [ %s ]" % (Result['Group'], Result['FrameId'], Result['Image']))
        print("  RMSE %s: %.8f" % (NameT1, RMSE_T1))
        print("  RMSE %s: %.8f" % (NameT2, RMSE_T2))
        print("  (%s - %s): %.8f" % (NameT2, NameT1, RMSE_T2 - RMSE_T1))
        print("  SSIM %s: %.8f" % (NameT1, SSIM_T1))
        print("  SSIM %s: %.8f" % (NameT2, SSIM_T2))
        print("  (%s - %s): %.8f" % (NameT2, NameT1, SSIM_T2 - SSIM_T1))

    def getExtension(vFileName):
        return os.path.splitext(vFileName)[-1]

    def copyCompareInfo(Result, Name):
        OutputDir = gOutputBaseDir + Name + "/"
        if not os.path.exists(OutputDir):
            os.makedirs(OutputDir)
        Ext = getExtension(Result['Image'])
        GTImage = vBaseDir + vDirGT + Result['Image']
        shutil.copyfile(GTImage, OutputDir + 'GroundTruth' + Ext)
        for i, Target in enumerate(vDirTarget):
            TargetResult = Result['TargetResult'][i]
            Ext = getExtension(TargetResult['Image'])
            TargetImage = vBaseDir + Target['Dir'] + TargetResult['Image']
            shutil.copyfile(TargetImage, OutputDir + Target['Name'] + Ext)
            OutHeatMapFileName = OutputDir + Target['Name'] + "_heatmap.png"
            getRMSE(GTImage, TargetImage, OutHeatMapFileName)

        RMSE_T1 = Result['TargetResult'][0]['RMSE']
        RMSE_T2 = Result['TargetResult'][1]['RMSE']
        SSIM_T1 = Result['TargetResult'][0]['SSIM']
        SSIM_T2 = Result['TargetResult'][1]['SSIM']
        FileRMSE = open(OutputDir + "Measure.txt", "w")
        FileRMSE.write("RMSE %s: %.8f\n" % (NameT1, RMSE_T1))
        FileRMSE.write("RMSE %s: %.8f\n" % (NameT2, RMSE_T2))
        FileRMSE.write("(%s - %s): %.8f\n\n" % (NameT2, NameT1, RMSE_T2 - RMSE_T1))
        FileRMSE.write("SSIM %s: %.8f\n" % (NameT1, SSIM_T1))
        FileRMSE.write("SSIM %s: %.8f\n" % (NameT2, SSIM_T2))
        FileRMSE.write("(%s - %s): %.8f\n" % (NameT2, NameT1, SSIM_T2 - SSIM_T1))
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
        printCompareInfo(CompareResult[BestIndex])
        copyCompareInfo(CompareResult[BestIndex], "Best %s" % vMeasureName)

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
        printCompareInfo(CompareResult[BestIndex])
        copyCompareInfo(CompareResult[BestIndex], "Best relative %s" % vMeasureName)

    findRelativeBest("RMSE", False)
    findRelativeBest("SSIM", True)



Exp = 0

if (Exp == 0):
    # Ghosting 
    BaseDir = "D:/Out/Ghosting/"
    DirGT = "GroundTruth/AccumulatePass-output/"
    DirTarget = [
        {
            'Name': 'SRGM',
            'Dir': "SRGM/STSM_BilateralFilter-Result/"
        },
        {
            'Name': 'TA',
            'Dir': "TA/STSM_BilateralFilter-Result/"
        },
    ]
    for Scene in ['Arcade']:
        for Type in ['Object', 'Light']:
            SubDir = Scene + "/" + Type + "/"
            run(BaseDir + SubDir, DirGT, DirTarget)

else:
    # Banding Compare Tranditional 
    BaseDir = "D:/Out/BandingCompareTranditional/"
    DirGT = "GroundTruth/AccumulatePass-output/"
    DirTarget = [
        {
            'Name': 'Random',
            'Dir': "Random/STSM_BilateralFilter-Result/"
        },
        {
            'Name': 'Tranditional_16',
            'Dir': "Tranditional_16/STSM_BilateralFilter-Result/"
        },
    ]
    run(BaseDir, DirGT, DirTarget)
os.system("pause")