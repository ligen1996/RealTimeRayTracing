from xml.dom.expatbuilder import ExpatBuilderNS
from falcor import *
import os
import Common

# manual config
# SRGM: adaptive on (Temporal Reuse and ReuseFactorEstimation)
# TA: adaptive off (Temporal Reuse and ReuseFactorEstimation)
# GroundTruth: adaptive off (Temporal Reuse and ReuseFactorEstimation)

ExpMainName = 'Ghosting'
ExpSubName = ['Object', 'Light', 'Camera'] # auto iteration all types
ExpAlgorithmName = ['SRGM','TA','GroundTruth']
ExpAlgorithmGraph = ['Ghosting-Object-NoSMV.py','Ghosting-Object.py','GroundTruth.py']
ExpIdx = 0

SceneSubPath = 'Experiment/' + ExpMainName + '/'

GraphName = ExpAlgorithmGraph[ExpIdx]

TotalFrame = 200
FramesToCapture = range(60,70)

m.clock.framerate = 120
m.clock.stop()

for i in range(len(ExpSubName)):
    if not Common.Record:
        if i == 0:
            print("Not Recording. So the rest graphs wont be loaded.")
        else:
            continue

    ExpType = ExpSubName[i]
    OutputPath = "d:/Out/" + ExpMainName + "/" + ExpType + "/" + ExpAlgorithmName[ExpIdx]
    if not os.path.exists(OutputPath):
        os.makedirs(OutputPath)
    m.frameCapture.outputDir = OutputPath

    SceneName = ExpType + '.pyscene'
    ExpName = ExpMainName + '-' + ExpType + '-' + ExpAlgorithmName[ExpIdx]

    m.script(Common.GraphPath + GraphName)
    m.loadScene(Common.ScenePath + SceneSubPath + SceneName)

    if (Common.Record):
        for i in range(TotalFrame):
            renderFrame()
            if i in FramesToCapture:
                # just for ground truth
                if ExpIdx == 2:
                    for j in range(0, 400):
                        renderFrame()
                m.frameCapture.baseFilename = ExpName + f"-{i:04d}"
                m.frameCapture.capture()
            m.clock.step()
        if not ExpIdx == 2:
            Common.keepOnlyFile(OutputPath, ["Result", "TR_Visibility"])
        Common.putIntoFolders(OutputPath)
        exit()
