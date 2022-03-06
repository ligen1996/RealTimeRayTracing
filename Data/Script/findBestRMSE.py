import os
import re
import shutil

useRelease = True

gComparerExe = "../../Bin/x64/%s/ImageCompare.exe" % ("Release" if useRelease else "Debug")


gBaseDir = "D:/Out/Ghosting/Object/"
gDirGT = "GroundTruth/AccumulatePass-output/"
gDirTarget = [
    {
        'Name': 'SRGM',
        'Dir': "SRGM/STSM_BilateralFilter-Result/"
    },
    {
        'Name': 'TA',
        'Dir': "TA/STSM_BilateralFilter-Result/"
    },
]

def getRMSE(vFileName1, vFileName2, vOutHeatMapFileName = None):
    cmd = "start /wait /b %s \"%s\" \"%s\" -m rmse" % (gComparerExe, vFileName1, vFileName2)
    if vOutHeatMapFileName:
        cmd += " -e \"%s\"" % vOutHeatMapFileName
    res = os.popen(cmd).read()
    return float(res)

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

CompareResult = []
GTImages = os.listdir(gBaseDir + gDirGT)
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
    
    for Target in gDirTarget:
        TargetImage = findSameFrame(gBaseDir + Target['Dir'], FrameId)
        if not TargetImage:
            print("No match found for file ", GTImage)
            continue
        RMSE = getRMSE(gBaseDir + gDirGT + GTImage, gBaseDir + Target['Dir'] + TargetImage)

        TargetResult = {}
        TargetResult['Name'] = Target['Name']
        TargetResult['Image'] = TargetImage
        TargetResult['RMSE'] = RMSE
        
        Result['TargetResult'].append(TargetResult)
    CompareResult.append(Result)

# print all
for i, Result in enumerate(CompareResult):
    print("Group %d, Frame %d: [ %s ]" % (Result['Group'], Result['FrameId'], Result['Image']))
    for TargetResult in Result['TargetResult']:
        print("  Target %s, RMSE = %.8f [ %s ]" % (TargetResult['Name'], TargetResult['RMSE'], TargetResult['Image']))


gOutputBaseDir = gBaseDir + "best/"
# find min
def printCompareInfo(Result):
    RMSE_SRGM = Result['TargetResult'][0]['RMSE']
    RMSE_TA = Result['TargetResult'][1]['RMSE']
    print("  Group %d, Frame %d [ %s ]" % (Result['Group'], Result['FrameId'], Result['Image']))
    print("  RMSE SRGM: %.8f" % (RMSE_SRGM))
    print("  RMSE TA: %.8f" % (RMSE_TA))
    print("  (TA - SRGM): %.8f" % (RMSE_TA - RMSE_SRGM))

def getExtension(vFileName):
    return os.path.splitext(vFileName)[-1]

def copyCompareInfo(Result, Name):
    OutputDir = gOutputBaseDir + Name + "/"
    if not os.path.exists(OutputDir):
        os.makedirs(OutputDir)
    Ext = getExtension(Result['Image'])
    GTImage = gBaseDir + gDirGT + Result['Image']
    shutil.copyfile(GTImage, OutputDir + 'GroundTruth' + Ext)
    for i, Target in enumerate(gDirTarget):
        TargetResult = Result['TargetResult'][i]
        Ext = getExtension(TargetResult['Image'])
        TargetImage = gBaseDir + Target['Dir'] + TargetResult['Image']
        shutil.copyfile(TargetImage, OutputDir + Target['Name'] + Ext)
        OutHeatMapFileName = OutputDir + Target['Name'] + "_heatmap.exr"
        getRMSE(GTImage, TargetImage, OutHeatMapFileName)

    RMSE_SRGM = Result['TargetResult'][0]['RMSE']
    RMSE_TA = Result['TargetResult'][1]['RMSE']
    FileRSME = open(OutputDir + "RMSE.txt", "w")
    FileRSME.write("RMSE SRGM: %.8f\n" % (RMSE_SRGM))
    FileRSME.write("RMSE TA: %.8f\n" % (RMSE_TA))
    FileRSME.write("(TA - SRGM): %.8f" % (RMSE_TA - RMSE_SRGM))
    FileRSME.close()

# min RMSE
MinIndex = -1
MinRSME = float("inf")
for i, Result in enumerate(CompareResult):
    RMSE_SRGM = Result['TargetResult'][0]['RMSE']
    if (MinRSME > RMSE_SRGM):
        MinIndex = i
        MinRSME = RMSE_SRGM
        
print("Min RSME:")
printCompareInfo(CompareResult[MinIndex])
copyCompareInfo(CompareResult[MinIndex], "Min RSME")

# max relative RMSE
MaxIndex = -1
MaxRelativeRSME = float("-inf")
for i, Result in enumerate(CompareResult):
    RMSE_SRGM = Result['TargetResult'][0]['RMSE']
    RMSE_TA = Result['TargetResult'][1]['RMSE']
    if (MaxRelativeRSME < (RMSE_TA - RMSE_SRGM)):
        MaxIndex = i
        MaxRelativeRSME = RMSE_TA - RMSE_SRGM
print("Max relative RSME:")
printCompareInfo(CompareResult[MaxIndex])
copyCompareInfo(CompareResult[MinIndex], "Max relative RSME")

os.system("pause")