from xml.dom.expatbuilder import ExpatBuilderNS
from falcor import *
import os
import Common

ExpMainName = 'BandingCompareSelf'
ExpAlgorithmName = ['Random','Layer']
ExpAlgorithmGraph = ['Ghosting-Object.py','Ghosting-Object-NoSMV.py']
ExpObjName = ['Dragon','Grid']
ExpIdx = 1

SceneSubPath = 'Experiment/' + ExpMainName + '/'

GraphName = ExpAlgorithmGraph[0]

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
    
    OutputPath = "d:/Out/" + ExpMainName + "/" + ExpAlgorithmName[ExpIdx] + "/" + ObjName
    if not os.path.exists(OutputPath):
        os.makedirs(OutputPath)
    m.frameCapture.outputDir = OutputPath

    SceneName = ObjName + '.pyscene'
    ExpName = ExpMainName + '-' + ExpAlgorithmName[ExpIdx] + "-" + ObjName

    m.script(Common.GraphPath + GraphName)
    m.loadScene(Common.ScenePath + SceneSubPath + SceneName)

    if (Common.Record):
        for i in range(TotalFrame):
            renderFrame()
            if i in FramesToCapture:
                m.frameCapture.baseFilename = ExpName + f"-{i:04d}"
                m.frameCapture.capture()
            m.clock.step()
        Common.keepOnlyFile(OutputPath, ["Result", "TR_Visibility"])
        Common.putIntoFolders(OutputPath)
        exit()
