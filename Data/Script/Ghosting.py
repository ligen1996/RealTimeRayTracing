import sys
sys.path.insert(0, 'D:/project/RealTimeRayTracing/Data/Script')

from xml.dom.expatbuilder import ExpatBuilderNS
from falcor import *
import os
import time
import Common

# SRGM: adaptive on (Temporal Reuse and ReuseFactorEstimation) filter iter=3
# TA: adaptive off (Temporal Reuse and ReuseFactorEstimation)
# GroundTruth: adaptive off (Temporal Reuse and ReuseFactorEstimation)

ExpMainName = 'Ghosting'
ExpAlgorithmNames = ['SRGM','TA','GroundTruth']
ExpAlgorithmGraphs = ['GraphSRGMFinal.py','GraphSRGMFinal.py','GroundTruth.py']
ExpMoveTypes = ['Object', 'Light']
ExpScenes = ['Grid', 'Robot', 'Pendulum']

SceneParentDir = Common.ScenePath + 'Experiment/' + ExpMainName + '/'

KeepList = ["Result", "TR_Visibility", "LTC"]

TotalFrame = 100
FramesToCapture = range(60, 70)
m.clock.framerate = 60

def updateParam(ExpName):
    if ExpName == 'GroundTruth':
        return

    graph = m.activeGraph
    PassReuse = graph.getPass("STSM_TemporalReuse")
    if ExpName == 'SRGM':
        PassReuse.AdaptiveAlpha = True
    elif ExpName == 'TA':
        PassReuse.AdaptiveAlpha = False

for ExpIdx, ExpAlgName in enumerate(ExpAlgorithmNames):
    GraphName = ExpAlgorithmGraphs[ExpIdx]
    if m.activeGraph:
        m.removeGraph(m.activeGraph)
    m.script(Common.GraphPath + GraphName) # load graph of algorithm
    updateParam(ExpAlgName)
    for Scene in ExpScenes:
        for MoveType in ExpMoveTypes:
            OutputPath = Common.OutDir + ExpMainName + "/" + Scene + "/" + MoveType + "/" + ExpAlgName
            if not os.path.exists(OutputPath):
                os.makedirs(OutputPath)
            m.frameCapture.reset()
            m.frameCapture.outputDir = OutputPath
            
            SceneFile = MoveType + Scene + '.pyscene'
            ExpName = ExpMainName + '-' + MoveType + '-' + ExpAlgName
            m.loadScene(SceneParentDir + SceneFile)
            if (Common.Record):
                m.clock.stop()
                for i in range(TotalFrame):
                    renderFrame()
                    if i in FramesToCapture:
                        # just for ground truth
                        if ExpAlgName == 'GroundTruth':
                            for j in range(0, 400):
                                renderFrame()
                        m.frameCapture.baseFilename = ExpName + f"-{i:04d}"
                        m.frameCapture.capture()
                    m.clock.step()
                time.sleep(10)
                if not ExpAlgName == 'GroundTruth':
                    Common.keepOnlyFile(OutputPath, KeepList)
                Common.putIntoFolders(OutputPath)
            else:
                m.clock.stop()
                for i in range(TotalFrame):
                    renderFrame()
                    if not i in FramesToCapture:
                        m.clock.step()
                    if i == FramesToCapture[0]:
                        time.sleep(5)
exit()