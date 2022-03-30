import Common

KeepList = ["Result", "TR_Visibility", "LTC"]
ExpMainName = 'SMV'
ExpAlgorithmNames = ['SMV','NoSMV','GroundTruth']
ExpMoveTypes = ['Object', 'Light']
ExpSceneNames = ['Grid', 'Dragon', 'Robot']

def cleanup(Path):
    try:
        if Path.find('GroundTruth') == -1:
            Common.keepOnlyFile(Path, KeepList)
        Common.putIntoFolders(Path)
        return True
    except Exception as e:
        print('异常：', e)
        return False

def getPaths():
    Paths = []
    for ExpIdx, ExpAlgName in enumerate(ExpAlgorithmNames):
        for Scene in ExpSceneNames:
            for MoveType in ExpMoveTypes:
                Paths.append(Common.OutDir + ExpMainName + "/" + Scene + "/" + MoveType + "/" + ExpAlgName)
    return Paths
                
Paths = getPaths()
for Path in Paths:
    while not cleanup(Path):
        i = input("整理失败，是否重试？Y/N")
        if (i == "N" or i == "n"):
            break