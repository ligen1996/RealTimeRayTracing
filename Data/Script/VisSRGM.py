from xml.dom.expatbuilder import ExpatBuilderNS
from falcor import *
import os
import Common

ExpMainName = 'VisSRGM'
GraphName = 'GraphVisSRGM.py'

SceneSubPath = 'Experiment/' + ExpMainName + '/'

TotalFrame = 200
FramesToCapture = range(60,70)

m.clock.framerate = 60
m.clock.stop()

SceneName = 'Grid.pyscene'
OutputPath = "d:/Out/" + ExpMainName + "/"
if not os.path.exists(OutputPath):
    os.makedirs(OutputPath)
m.frameCapture.outputDir = OutputPath
ExpName = ExpMainName
m.script(Common.GraphPath + GraphName)
m.loadScene(Common.ScenePath + SceneSubPath + SceneName)
if (Common.Record):
    for i in range(TotalFrame):
        renderFrame()
        if i in FramesToCapture:
            m.frameCapture.baseFilename = ExpName + f"-{i:04d}"
            m.frameCapture.capture()
        m.clock.step()
    Common.keepOnlyFile(OutputPath, ["Result", "TR_Visibility", "Variation", "VarOfVar", "Out"])
    Common.putIntoFolders(OutputPath)
    exit()
