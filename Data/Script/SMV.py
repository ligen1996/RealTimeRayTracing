import sys
sys.path.insert(0, 'D:/project/RealTimeRayTracing/Data/Script')

from xml.dom.expatbuilder import ExpatBuilderNS
from falcor import *
import os
import time
import Common

# 50 frames for ploting
# no adapative
# SMV: alpha = 0.05 to show ghosting
# NOSMV: alpha = 0.08 to avoid too ghosting
# SMV: use shadow motion vector 
# NOSMV: normal motion vector

ExpMainName = 'SMV'
ExpAlgorithmNames = ['SMV','NoSMV','GroundTruth']
ExpAlgorithmGraphs = ['GraphSRGMFinal.py', 'GraphSRGMFinal.py', 'GroundTruth.py']
ExpMoveTypes = ['Object', 'Light']
ExpSceneNames = ['Grid', 'Dragon', 'Robot']
ExpMoveTypes = ['Light']
ExpScenes = ['Dragon']

SceneParentDir = Common.ScenePath + 'Experiment/SMV/'

KeepList = ["Result", "TR_Visibility", "LTC"]

TotalFrame = 200
FramesToCapture = range(120, 170)
m.clock.framerate = 120

def updateParam(ExpName, SceneName, MoveType):
    graph = m.activeGraph

    if ExpName == 'GroundTruth':
        PassVis = graph.getPass("STSM_CalculateVisibility")
        PassVis.RandomSelection = False
        PassVis.SelectNum = 16
        return
        
    PassSMV = graph.getPass("MS_Visibility")
    PassReuse = graph.getPass("STSM_TemporalReuse")
    PassReuse.AdaptiveAlpha = False
    if ExpName == 'SMV':
        PassReuse.Alpha = 0.05
        PassSMV.UseSMV = True
    elif ExpName == 'NoSMV':
        if SceneName == 'Grid':
            PassReuse.Alpha = 0.1
        else:
            PassReuse.Alpha = 0.05
        PassSMV.UseSMV = False

AllOutputPaths = []
for ExpIdx, ExpAlgName in enumerate(ExpAlgorithmNames):
    GraphName = ExpAlgorithmGraphs[ExpIdx]
    if m.activeGraph:
        m.removeGraph(m.activeGraph)
    m.script(Common.GraphPath + GraphName) # load graph of algorithm
    
    for Scene in ExpSceneNames:
        for MoveType in ExpMoveTypes:
            updateParam(ExpAlgName, Scene, MoveType)
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