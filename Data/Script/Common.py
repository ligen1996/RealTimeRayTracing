import os
import re
import shutil
import json

Record = True # if record and save frames
GraphPath = '../../Data/Graph/'
ScenePath = '../../Data/Scene/'

def keepOnlyFile(vDir, vKeywords):
    FileNames = os.listdir(vDir)
    for FileName in FileNames:
        if (os.path.isdir(FileName)):
            continue
        for Keyword in vKeywords:
            if FileName.find(Keyword) >= 0:
                break
        else:
            os.remove(vDir + "/" + FileName)

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
        shutil.move(vDir + "/" + FileName, FolderPath + "/" + FileName, )

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