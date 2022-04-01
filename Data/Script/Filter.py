from xml.dom.expatbuilder import ExpatBuilderNS
from falcor import *
import os
import Common
import time

# test convergen and result of spacial filter
# Filtered: enable space filter
# Original: disable space filter
# Tranditional: no srgm adaptive, no random SM, no smv, no space reuse

ExpMainName = 'Filter'
ExpAlgorithmNames = ['Filtered', 'Original', 'Tranditional', 'GroundTruth']
ExpAlgorithmGraphs = ['GraphSRGMFinal.py','GraphSRGMFinal.py','GraphSRGMFinal.py','GroundTruth.py']
ExpSceneNames = ['GridObserve', 'DragonObserve', 'RobotObserve']
ExpSceneNames = ['RobotObserve']

SceneParentDir = Common.ScenePath + 'Experiment/Banding/'

KeepList = ["Result", "TR_Visibility", "LTC"]

TotalFrame = 100
FramesToCapture = range(0, 100)
m.clock.framerate = 60

def updateParam(ExpName, SceneName):
    if (SceneName == "RobotObserve"):
        m.scene.lights[0].openingAngle = 3.1415926 * 50 / 180 # small fov to avoid blocking

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
    if  ExpName == 'Filtered' or ExpName == 'Original':
        PassVis.RandomSelection = True
        PassVis.SelectNum = 8
        PassReuse.AdaptiveAlpha = True
        PassReuse.Alpha = 0.1
        PassSMV.UseSMV = True
        if ExpName == 'Filtered':
            PassFilter.Enable = True
        elif ExpName == 'Original':
            PassFilter.Enable = False
    elif ExpName == 'Tranditional':
        PassFilter.Enable = False
        PassVis.RandomSelection = False
        PassVis.SelectNum = 16
        PassReuse.AdaptiveAlpha = False
        PassReuse.Alpha = 0.5
        PassSMV.UseSMV = False
    else:
        raise

AllOutputPaths = []
for ExpIdx, ExpAlgName in enumerate(ExpAlgorithmNames):
    GraphName = ExpAlgorithmGraphs[ExpIdx]
    if m.activeGraph:
        m.removeGraph(m.activeGraph)
    m.script(Common.GraphPath + GraphName) # load graph of algorithm
    
    for Scene in ExpSceneNames:
        OutputPath = Common.OutDir + ExpMainName + "/" + Scene + "/" + ExpAlgName
        if not os.path.exists(OutputPath):
            os.makedirs(OutputPath)
        m.frameCapture.reset()
        m.frameCapture.outputDir = OutputPath
        
        SceneFile = Scene + '.pyscene'
        ExpName = ExpMainName + '-' + ExpAlgName
        m.loadScene(SceneParentDir + SceneFile)
        updateParam(ExpAlgName, Scene)
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