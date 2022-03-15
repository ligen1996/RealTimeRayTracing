from xml.dom.expatbuilder import ExpatBuilderNS
from falcor import *
import os
import Common
import time

# random: enable random
# Tranditional: disable random

ExpMainName = 'BandingCompareSelf'
ExpAlgorithmName = ['Random', 'Banding', 'GroundTruth']
ExpAlgorithmGraph = ['Ghosting-Object-NoSMV.py','Ghosting-Object-NoSMV.py','GroundTruth.py']
ExpSceneName = ['GridObserve', 'DragonObserve', 'ArcadeObserve']
ExpIdx = 2

SceneSubPath = 'Experiment/BandingCompareTranditional/'
ExpName = ExpMainName + '-' + ExpAlgorithmName[ExpIdx]

GraphName = ExpAlgorithmGraph[ExpIdx]
m.script(Common.GraphPath + GraphName)

TotalFrame = 80
FramesToCapture = range(60, 70)
m.clock.framerate = 120

for Scene in ExpSceneName:
    SceneName = Scene + '.pyscene'
    OutputPath = "d:/Out/" + ExpMainName + "/" + Scene + "/" + ExpAlgorithmName[ExpIdx]
    
    Common.removeDir(OutputPath)
    os.makedirs(OutputPath)

    m.loadScene(Common.ScenePath + SceneSubPath + SceneName)

    if (Common.Record):
        m.clock.stop()
        m.frameCapture.outputDir = OutputPath

        for i in range(TotalFrame):
            renderFrame()
            if i in FramesToCapture:
                # just for ground truth
                if ExpIdx == 2:
                    for j in range(0, 500):
                        renderFrame()
                m.frameCapture.baseFilename = ExpName + f"-{i:04d}"
                m.frameCapture.capture()
            m.clock.step()
        time.sleep(2)
        if not ExpIdx == 2:
            Common.keepOnlyFile(OutputPath, ["Result", "TR_Visibility"])
        Common.putIntoFolders(OutputPath)

exit()
