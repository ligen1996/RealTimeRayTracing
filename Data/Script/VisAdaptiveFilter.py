from xml.dom.expatbuilder import ExpatBuilderNS
from falcor import *
import os
import Common

ExpMainName = 'VisAdaptiveFilter'
ExpAlgorithmGraph = ['Ghosting-Object.py','Ghosting-Object-NoSMV.py']

SceneSubPath = 'Experiment/' + ExpMainName + '/'

GraphName = ExpAlgorithmGraph[0]
SceneName = 'Grid.pyscene'

OutputPath = "d:/Out/" + ExpMainName
if not os.path.exists(OutputPath):
    os.makedirs(OutputPath)

ExpName = ExpMainName
TotalFrame = 200
FramesToCapture = range(10, 110, 10)

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
    Common.keepOnlyFile(OutputPath, ["STSM_BilateralFilter", "TR_Visibility"])
    Common.putIntoFolders(OutputPath)
    exit()
