from xml.dom.expatbuilder import ExpatBuilderNS
from falcor import *
import os
import time
import Common

# no adapative, alpha = 0.1
# banding?

ExpMainName = 'SMV'
ExpMoveType = ['Object', 'Light']
ExpAlgorithmName = ['SMV','NoSMV','GroundTruth']
ExpAlgorithmGraph = ['Ghosting-Object.py','Ghosting-Object-NoSMV.py','GroundTruth.py']

ExpSceneName = ['Grid', 'Dragon', 'Arcade']

SceneSubPath = 'Experiment/Ghosting/'

TotalFrame = 200
FramesToCapture = range(120,130)

m.clock.framerate = 60
m.clock.stop()

for SceneName in ExpSceneName:
    for MoveType in ExpMoveType:
        SceneFile = MoveType + SceneName + '.pyscene'
        m.loadScene(Common.ScenePath + SceneSubPath + SceneFile)
        for AlgIdx, Alg in enumerate(ExpAlgorithmName):
            GraphName = ExpAlgorithmGraph[AlgIdx]
            if m.activeGraph:
                m.removeGraph(m.activeGraph)
            m.script(Common.GraphPath + GraphName)
            if not Common.Record:
                if AlgIdx == 0:
                    print("Not Recording. So the rest graphs wont be loaded.")
                else:
                    continue

            OutputPath = "d:/Out/" + ExpMainName + "/" + SceneName + "/" + MoveType + "/" + Alg
            if not os.path.exists(OutputPath):
                os.makedirs(OutputPath)
            m.frameCapture.reset()
            m.frameCapture.outputDir = OutputPath

            if (Common.Record):
                m.clock.stop()
                for i in range(TotalFrame):
                    renderFrame()
                    if i in FramesToCapture:
                        # just for ground truth
                        if AlgIdx == 2:
                            for j in range(0, 400):
                                renderFrame()
                        m.frameCapture.baseFilename = ExpMainName + f"-{i:04d}"
                        m.frameCapture.capture()
                    m.clock.step()
                time.sleep(1)
                if not AlgIdx == 2:
                    Common.keepOnlyFile(OutputPath, ["Result", "TR_Visibility"])
                Common.putIntoFolders(OutputPath)

exit()