import sys
sys.path.insert(0, 'D:/project/RealTimeRayTracing/Data/Script')

from xml.dom.expatbuilder import ExpatBuilderNS
from falcor import *
import os
import time
import Common

# SMV??? need to modify vis all graph
# turn off reliability
# Adaptive: enable adaptive, alpha = 0.1
# No adaptive: disable adaptive, alpha = 0.05
# merge channel = ddv * 10 green

ExpMainName = 'VisAll'
ExpAlgorithmNames = ['VisAdaptive', 'VisNoAdaptive', 'GroundTruth'] 
ExpAlgorithmGraphs = ['GraphVisAll.py', 'GraphVisAll.py', 'GroundTruth.py']
ExpMoveTypes = ['Object', 'Light']
ExpScenes = ['Grid', 'Dragon', 'Robot']

SceneParentDir = Common.ScenePath + 'Experiment/Ghosting/'

TotalFrame = 100
FramesToCapture = range(60, 70)
m.clock.framerate = 60

KeepList = ["Result", "TR_Visibility", "Variation", "VarOfVar", "BilateralFilter.Debug", "TemporalReuse.Debug", "MergeChannels", "LTC"]

def updateParam(ExpName):
    graph = m.activeGraph

    if ExpName == 'GroundTruth':
        PassVis = graph.getPass("STSM_CalculateVisibility")
        PassVis.RandomSelection = False
        PassVis.SelectNum = 16
        return
        
    PassReuse = graph.getPass("STSM_TemporalReuse")
    PassSRGM = graph.getPass("STSM_ReuseFactorEstimation")
    PassSRGM.ReliabilityStrength = 0.0
    # TODO: SMV?
    if ExpName == 'VisAdaptive':
        PassReuse.AdaptiveAlpha = True
        PassReuse.Alpha = 0.1
    elif ExpName == 'VisNoAdaptive':
        PassReuse.AdaptiveAlpha = False
        PassReuse.Alpha = 0.05

AllOutputPaths = []
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
                AllOutputPaths.append(OutputPath)
            else:
                input("Not recording. Press Enter to next experiment")

time.sleep(10)
Common.cleanupAll(AllOutputPaths, KeepList)
exit()     