from xml.dom.expatbuilder import ExpatBuilderNS
from falcor import *
import os
import Common

ExpMainName = 'Ghosting'
ExpSubName = 'Object'
ExpAlgorithmName = ['SRGM','TA','GroundTruth']
ExpIdx = 0

SceneSubPath = 'Experiment/' + ExpMainName + '/'

GraphName = 'GraphSTSM_MS_FINAL.py'
SceneName = ExpSubName+'.pyscene'

OutputPath = "d:/Out/" + ExpMainName + "/" + ExpObjName[ObjId]
if not os.path.exists(OutputPath):
    os.makedirs(OutputPath)

ExpName = ExpMainName + '-' + ExpSubName + '-' + ExpAlgorithmName[ExpIdx]
TotalFrame = 300
FramesToCapture = range(200,210)

m.script(Common.GraphPath + GraphName)
m.loadScene(Common.ScenePath + SceneSubPath + SceneName)

m.clock.framerate = 120

if (Common.Record):
    m.clock.stop()
    m.frameCapture.outputDir = OutputPath

    for i in range(TotalFrame):
        renderFrame()
        if i in FramesToCapture:
            # just for ground truth
            if ExpIdx == 2 :
                for j in range(0,100):
                    renderFrame()
            m.frameCapture.baseFilename = ExpName + f"-{i:04d}"
            m.frameCapture.capture()
        m.clock.step()
    exit()