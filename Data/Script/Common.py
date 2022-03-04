import os
import re
import shutil

Record = True # if record and save frames
GraphPath = '../../Data/Graph/'
ScenePath = '../../Data/Scene/'

def keepOnlyFile(vDir, vKeywords):
    FileNames = os.listdir(vDir)
    for FileName in FileNames:
        for Keyword in vKeywords:
            if FileName.find(Keyword) >= 0:
                break
        else:
            os.remove(vDir + "/" + FileName)

def putIntoFolders(vDir):
    FileNames = os.listdir(vDir)
    for FileName in FileNames:
        Match = re.search(r"([^\.]+)\.([^\.]+)\.\d+\.exr", FileName)
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
