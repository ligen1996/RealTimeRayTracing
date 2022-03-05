import os
import re

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

def extractFrameId(vFileName):
    Match = re.search(r".*\.(\d+)\.exr", vFileName)
    if (Match == None):
        return None
    FrameId = int(Match.group(1))
    return FrameId

def findSameFrame(vDir, vFrameId):
    FileNames = os.listdir(vDir)
    for FileName in FileNames:
        if (os.path.isdir(FileName)):
            continue
        Match = re.search(r".*\." + str(vFrameId) + r"\.exr", FileName)
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
        cmd = "start /wait /b %s %s %s" % (gComparerExe, gBaseDir + gDirGT + GTImage, gBaseDir + Target['Dir'] + TargetImage)
        res = os.popen(cmd).read()
        RMSE = float(res)

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

# find min
def printCompareInfo(Result):
    RMSE_SRGM = Result['TargetResult'][0]['RMSE']
    RMSE_TA = Result['TargetResult'][1]['RMSE']
    print("  Group %d, Frame %d [ %s ]" % (Result['Group'], Result['FrameId'], Result['Image']))
    print("  RMSE SRGM: %.8f" % (RMSE_SRGM))
    print("  RMSE TA: %.8f" % (RMSE_TA))
    print("  (TA - SRGM): %.8f" % (RMSE_TA - RMSE_SRGM))

# min RMSE
MinIndex = -1
MinRSME = 1.0
for i, Result in enumerate(CompareResult):
    RMSE_SRGM = Result['TargetResult'][0]['RMSE']
    if (MinRSME > RMSE_SRGM):
        MinIndex = i
        MinRSME = RMSE_SRGM
        
print("Min RSME:")
printCompareInfo(CompareResult[MinIndex])

# max relative RMSE
MaxIndex = -1
MaxRelativeRSME = -1
for i, Result in enumerate(CompareResult):
    RMSE_SRGM = Result['TargetResult'][0]['RMSE']
    RMSE_TA = Result['TargetResult'][1]['RMSE']
    if (MaxRelativeRSME < (RMSE_TA - RMSE_SRGM)):
        MaxIndex = i
        MaxRelativeRSME = RMSE_TA - RMSE_SRGM
print("Min relative RSME:")
printCompareInfo(CompareResult[MaxIndex])

os.system("pause")