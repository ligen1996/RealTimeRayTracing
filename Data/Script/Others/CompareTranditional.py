import sys
sys.path.insert(0, 'D:/project/RealTimeRayTracing/Data/Script')

from xml.dom.expatbuilder import ExpatBuilderNS
from falcor import *
import os
import Common
import time

# SRGM: all on
# Tranditional: no srgm adaptive, no random SM, no smv, no space reuse

ExpMainName = 'CompareTranditional'
ExpAlgorithmNames = ['SRGM', 'Tranditional', 'GroundTruth']
ExpAlgorithmGraphs = ['GraphSRGMFinal.py', 'GraphSRGMFinal.py', 'GroundTruth.py']
ExpMoveTypes = ['Object', 'Light']
ExpSceneNames = ['Grid', 'Dragon', 'Robot']

SceneParentDir = Common.ScenePath + 'Experiment/Ghosting/'

KeepList = ["Result", "TR_Visibility", "LTC"]

TotalFrame = 100
FramesToCapture = range(60, 70)
m.clock.framerate = 60

def updateParam(ExpName):
    graph = m.activeGraph

    if ExpName == 'GroundTruth':
        PassVis = graph.getPass("STSM_CalculateVisibility")
        PassVis.RandomSelection = False
        PassVis.SelectNum = 16
        return

    PassSMV = graph.getPass("MS_Visibility")
    PassVis = graph.getPass("STSM_CalculateVisibility")
    PassReuse = graph.getPass("STSM_TemporalReuse")
    PassFilter = graph.getPass("STSM_BilateralFilter")
    if ExpName == 'SRGM':
        PassVis.RandomSelection = True
        PassVis.SelectNum = 8
        PassReuse.AdaptiveAlpha = True
        PassReuse.Alpha = 0.1
        PassSMV.UseSMV = True
        PassFilter.Enable = True
    elif ExpName == 'Tranditional':
        PassVis.RandomSelection = False
        PassVis.SelectNum = 16
        PassReuse.AdaptiveAlpha = False
        PassReuse.Alpha = 0.1
        PassSMV.UseSMV = False
        PassFilter.Enable = False

AllOutputPaths = []
for ExpIdx, ExpAlgName in enumerate(ExpAlgorithmNames):
    GraphName = ExpAlgorithmGraphs[ExpIdx]
    if m.activeGraph:
        m.removeGraph(m.activeGraph)
    m.script(Common.GraphPath + GraphName) # load graph of algorithm
    
    for Scene in ExpSceneNames:
        for MoveType in ExpMoveTypes:
            updateParam(ExpAlgName)
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