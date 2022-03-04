from xml.dom.expatbuilder import ExpatBuilderNS
from falcor import *
import os
import Common

ExpMainName = 'Ghosting'
ExpSubName = 'Object'
ExpAlgorithmName = ['SRGM','TA','GroundTruth']
ExpAlgorithmGraph = ['Ghosting-Object-NoSMV.py','Ghosting-Object.py','GroundTruth.py']
ExpIdx = 0

SceneSubPath = 'Experiment/' + ExpMainName + '/'

GraphName = ExpAlgorithmGraph[ExpIdx]
SceneName = ExpSubName+'.pyscene'

OutputPath = "d:/Out/" + ExpMainName + "/" + ExpSubName
if not os.path.exists(OutputPath):
    os.makedirs(OutputPath)

ExpName = ExpMainName + '-' + ExpSubName + '-' + ExpAlgorithmName[ExpIdx]
TotalFrame = 200
FramesToCapture = range(60,70)

m.script(Common.GraphPath + GraphName)
m.loadScene(Common.ScenePath + SceneSubPath + SceneName)

m.clock.framerate = 120

m.clock.stop()
m.frameCapture.outputDir = OutputPath

if (Common.Record):
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