from xml.dom.expatbuilder import ExpatBuilderNS
from falcor import *
import os
import Common

ExpMainName = 'Flickering'
ExpSubName = ['Dynamic','Static']
ExpObjName = ['Dragon','Grid']
ExpIdx = 1
RepeatNum = 3
if(ExpIdx == 1):
    RepeatNum = 1

SceneSubPath = 'Experiment/' + ExpMainName + '/'
GraphName = 'Ghosting-Object-NoSMV.py'

TotalFrame = 200
FramesToCapture = range(60,70)
m.clock.framerate = 120

# iter all object
for i in range(len(ExpObjName)):
    ObjName = ExpObjName[i]
    if not Common.Record:
        if i == 0:
            print("Not Recording. So the rest graphs wont be loaded.")
        else:
            continue
    
    OutputPath = "d:/Out/" + ExpMainName + "/" + ExpSubName[ExpIdx] + "/" + ObjName
    if not os.path.exists(OutputPath):
        os.makedirs(OutputPath)
    m.frameCapture.outputDir = OutputPath

    SceneName = ObjName + '.pyscene'
    ExpName = ExpMainName + '-' + ExpSubName[ExpIdx] + "-" + ObjName

    m.script(Common.GraphPath + GraphName)
    m.loadScene(Common.ScenePath + SceneSubPath + SceneName)

    if (Common.Record):
        for j in range(RepeatNum):
            m.clock.stop()
            for i in range(TotalFrame):
                renderFrame()
                if i in FramesToCapture:
                    m.frameCapture.baseFilename = ExpName + f"-{i:04d}" + '-' + f"{j:d}"
                    m.frameCapture.capture()
                if(ExpIdx == 0):
                    m.clock.step()
        Common.keepOnlyFile(OutputPath, ["Result", "TR_Visibility"])
        Common.putIntoFolders(OutputPath)
        exit()
