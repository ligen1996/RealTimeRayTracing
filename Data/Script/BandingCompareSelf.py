from xml.dom.expatbuilder import ExpatBuilderNS
from falcor import *
import os
import Common

ExpMainName = 'BandingCompareSelf'
ExpAlgorithmName = ['Random','Layer']
ExpAlgorithmGraph = ['Ghosting-Object.py','Ghosting-Object-NoSMV.py']
ExpObjName = ['Dragon','Grid']
ExpIdx = 0
ObjId = 1

SceneSubPath = 'Experiment/' + ExpMainName + '/'

GraphName = ExpAlgorithmGraph[0]
SceneName = ExpObjName[ObjId]+'.pyscene'

OutputPath = "d:/Out/" + ExpMainName + "/" + ExpObjName[ObjId]
if not os.path.exists(OutputPath):
    os.makedirs(OutputPath)

ExpName = ExpMainName + '-' + ExpObjName[ObjId] + '-' + ExpAlgorithmName[ExpIdx]
TotalFrame = 200
FramesToCapture = range(60,70)

m.script(Common.GraphPath + GraphName)
m.loadScene(Common.ScenePath + SceneSubPath + SceneName)

m.clock.framerate = 120

if (Common.Record):
    m.clock.stop()
    m.frameCapture.outputDir = OutputPath

    for i in range(TotalFrame):
        renderFrame()
        if i in FramesToCapture:
            m.frameCapture.baseFilename = ExpName + f"-{i:04d}"
            m.frameCapture.capture()
        m.clock.step()
    exit()