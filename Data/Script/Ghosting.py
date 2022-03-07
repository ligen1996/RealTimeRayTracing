from xml.dom.expatbuilder import ExpatBuilderNS
from falcor import *
import os
import time
import Common

# manual config
# use no lagging
# SRGM: adaptive on (Temporal Reuse and ReuseFactorEstimation)
# TA: adaptive off (Temporal Reuse and ReuseFactorEstimation)
# GroundTruth: adaptive off (Temporal Reuse and ReuseFactorEstimation)

# for grid, object, to avoid lagging, require diffrent params and low filter iteration
# for dragon, use no smv and low realiability

ExpMainName = 'Ghosting'
ExpSubName = ['Object', 'Light'] # auto iteration all types
ExpAlgorithmName = ['SRGM','TA','GroundTruth']
ExpAlgorithmGraph = ['Ghosting-Object-NoSMV.py','Ghosting-Object-NoSMV.py','GroundTruth.py']
ExpIdx = 0

ExpSceneName = ['Grid', 'Dragon', 'Arcade']
SceneName = ExpSceneName[2]

SceneSubPath = 'Experiment/' + ExpMainName + '/'

GraphName = ExpAlgorithmGraph[ExpIdx]

TotalFrame = 200
FramesToCapture = range(120,130)

m.clock.framerate = 120
m.clock.stop()

for i in range(len(ExpSubName)):
    if not Common.Record:
        if i == 0:
            print("Not Recording. So the rest graphs wont be loaded.")
        else:
            continue

    ExpType = ExpSubName[i]
    OutputPath = "d:/Out/" + ExpMainName + "/" + SceneName + "/" + ExpType + "/" + ExpAlgorithmName[ExpIdx]
    if not os.path.exists(OutputPath):
        os.makedirs(OutputPath)
    m.frameCapture.outputDir = OutputPath

    SceneFile = ExpType + SceneName + '.pyscene'
    ExpName = ExpMainName + '-' + ExpType + '-' + ExpAlgorithmName[ExpIdx]

    m.script(Common.GraphPath + GraphName)
    m.loadScene(Common.ScenePath + SceneSubPath + SceneFile)

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
        time.sleep(1)
        if not ExpIdx == 2:
            Common.keepOnlyFile(OutputPath, ["Result", "TR_Visibility"])
        Common.putIntoFolders(OutputPath)
        exit()
