from xml.dom.expatbuilder import ExpatBuilderNS
from falcor import *
import os
import Common
import time

# random: enable random, 2 samples, enable adaptive
# Tranditional: disable random, disable adaptive, 16 samples and 256 samples, big alpha = 0.3 (change loaded param file)

ExpMainName = 'BandingCompareTranditional'
ExpAlgorithmName = ['Random', 'Tranditional_16', 'GroundTruth']
ExpAlgorithmGraph = ['Ghosting-Object.py','Ghosting-Object.py','GroundTruth.py']
ExpIdx = 0

SceneSubPath = 'Experiment/' + ExpMainName + '/'

GraphName = ExpAlgorithmGraph[ExpIdx]
SceneName = 'Grid.pyscene'

OutputPath = "d:/Out/" + ExpMainName + "/" + ExpAlgorithmName[ExpIdx]
if not os.path.exists(OutputPath):
    os.makedirs(OutputPath)

ExpName = ExpMainName + '-' + ExpAlgorithmName[ExpIdx]
TotalFrame = 80
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
            # just for ground truth
            if ExpIdx == 2:
                for j in range(0, 150):
                    renderFrame()
            m.frameCapture.baseFilename = ExpName + f"-{i:04d}"
            m.frameCapture.capture()
        m.clock.step()
    time.sleep(1)
    if not ExpIdx == 2:
        Common.keepOnlyFile(OutputPath, ["Result", "TR_Visibility"])
    Common.putIntoFolders(OutputPath)
    exit()
