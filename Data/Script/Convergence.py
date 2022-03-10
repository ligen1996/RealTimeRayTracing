from xml.dom.expatbuilder import ExpatBuilderNS
from falcor import *
import os
import Common
import time

# random: enable random, adaptive
# Tranditional: disable random, disable adaptive, 16 samples, big alpha = 0.4 (change loaded param file)

ExpMainName = 'Convergence'
ExpMoveType = ['Object', 'Light'] # auto iteration all types
ExpAlgorithmName = ['Random', 'Tranditional_16', 'GroundTruth']
ExpAlgorithmGraph = ['Ghosting-Object-NOSMV.py','Ghosting-Object-NOSMV.py','GroundTruth.py']
ExpSceneName = ['Grid', 'Dragon', 'Arcade']
ExpIdx = 1

SceneSubPath = 'Experiment/Convergence/'
ExpName = ExpMainName + '-' + ExpAlgorithmName[ExpIdx]

GraphName = ExpAlgorithmGraph[ExpIdx]
m.script(Common.GraphPath + GraphName)

TotalFrame = 120
FramesToCapture = range(0, 40)
m.clock.framerate = 120

for SceneName in ExpSceneName:
    for MoveType in ExpMoveType:
        SceneFile = MoveType + SceneName + '.pyscene'
        OutputPath = "d:/Out/" + ExpMainName + "/" + SceneName + "/" + MoveType + "/" + ExpAlgorithmName[ExpIdx]
        
        Common.removeDir(OutputPath)
        os.makedirs(OutputPath)

        m.loadScene(Common.ScenePath + SceneSubPath + SceneFile)

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
                # m.clock.step()
            time.sleep(2)
            if not ExpIdx == 2:
                Common.keepOnlyFile(OutputPath, ["Result", "TR_Visibility"])
            Common.putIntoFolders(OutputPath)

exit()
