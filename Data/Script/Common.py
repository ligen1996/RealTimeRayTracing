import os
import re
import shutil
import json

Record = True # if record and save frames
OutDir = 'E:/Out/'
GraphPath = '../../Data/Graph/'
ScenePath = '../../Data/Scene/'

def keepOnlyFile(vDir, vKeywords):
    FileNames = os.listdir(vDir)
    for FileName in FileNames:
        Path = vDir + "/" + FileName
        if (os.path.isdir(Path)):
            continue
        for Keyword in vKeywords:
            if FileName.find(Keyword) >= 0:
                break
        else:
            if (os.path.exists(Path)):
                if (os.access(Path, os.W_OK)):
                    os.remove(Path)
                else:
                    print("Cant access path: ", Path)

def putIntoFolders(vDir):
    FileNames = os.listdir(vDir)
    for FileName in FileNames:
        if (os.path.isdir(vDir + "/" + FileName)):
            continue
        Match = re.search(r"([^\.]+)\.([^\.]+)\.\d+\.(exr|png)", FileName)
        if (Match == None):
            print("错误：未正确解析文件名[", FileName, "]")
            continue
        PassName = Match.group(1) 
        OutputName = Match.group(2)
        FolderName = PassName + "-" + OutputName
        FolderPath = vDir + "/" + FolderName
        if not os.path.exists(FolderPath):
            os.makedirs(FolderPath)
        Path = vDir + "/" + FileName
        if (os.path.exists(Path)):
            if (os.access(Path, os.W_OK)):
                shutil.move(Path, FolderPath + "/" + FileName, )
            else:
                print("Cant access path: ", Path)


def writeJSON(obj, fileName):
    f = open(fileName, "w")
    f.write(json.dumps(obj, indent=2))
    f.close()

def removeDir(vDir):
    if not os.path.exists(vDir):
        return
    if not os.path.isdir(vDir):
        raise "remove non-directory"
    shutil.rmtree(vDir)

def cleanup(vPath, vKeepList):
    try:
        if vPath.find('GroundTruth') == -1:
            keepOnlyFile(vPath, vKeepList)
        putIntoFolders(vPath)
        return True
    except Exception as e:
        print('异常：', e)
        return False

def cleanupAll(vPaths, vKeepList):
    for Path in vPaths:
        while not cleanup(Path, vKeepList):
            i = input("整理失败，是否重试？Y/N ")
            if (i == "N" or i == "n"):
                break