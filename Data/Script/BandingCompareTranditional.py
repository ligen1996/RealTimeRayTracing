from xml.dom.expatbuilder import ExpatBuilderNS
from falcor import *
import os
import Common

# random: enable random, 2 samples
# Tranditional: disable random, 16 samples and 256 samples

ExpMainName = 'BandingCompareTranditional'
ExpAlgorithmName = ['Random', 'Tranditional_16', 'Tranditional_256']
ExpAlgorithmGraph = ['Ghosting-Object.py','Ghosting-Object-NoSMV.py']
ExpIdx = 0

SceneSubPath = 'Experiment/' + ExpMainName + '/'

GraphName = ExpAlgorithmGraph[0]
SceneName = 'Grid.pyscene'

OutputPath = "d:/Out/" + ExpMainName + "/" + ExpAlgorithmName[ExpIdx]
if not os.path.exists(OutputPath):
    os.makedirs(OutputPath)

ExpName = ExpMainName + '-' + ExpAlgorithmName[ExpIdx]
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
    Common.keepOnlyFile(OutputPath, ["Result", "TR_Visibility"])
    Common.putIntoFolders(OutputPath)
    exit()
