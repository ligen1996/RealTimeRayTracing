import sys
sys.path.insert(0, 'D:/project/RealTimeRayTracing/Data/Script')

from xml.dom.expatbuilder import ExpatBuilderNS
from falcor import *
import os
import Common
import time

# SRGM_Adaptive: all on
# SRGM_NoAdaptive: all on, but no adaptive
# Tranditional: no srgm adaptive, no random SM, no smv, no space reuse

ExpMainName = 'CompareAll'
ExpAlgorithmGraph = 'Efficiency.py'
ExpSceneNames = ['LightGrid', 'LightDragon', 'LightRobot', 'ObjectGrid', 'ObjectDragon', 'ObjectRobot']
ExpMoveTypes = ['Static', 'Static', 'Static', 'Dynamic', 'Dynamic', 'Dynamic']

SceneParentDir = Common.ScenePath + 'Experiment/Ghosting/'

TotalFrame = 2000
m.clock.framerate = 60

def updateParam():
    graph = m.activeGraph

    PassSMV = graph.getPass("MS_Visibility")
    PassVis = graph.getPass("STSM_CalculateVisibility")
    PassReuse = graph.getPass("STSM_TemporalReuse")
    PassFilter = graph.getPass("STSM_BilateralFilter")

    PassFilter.Enable = False
    PassFilter.Iteration = 1


if m.activeGraph:
    m.removeGraph(m.activeGraph)
m.script(Common.GraphPath + ExpAlgorithmGraph) # load graph of algorithm
updateParam()

for i, Scene in enumerate(ExpSceneNames):
    MoveType = ExpMoveTypes[i]
        
    OutputPath = Common.OutDir + ExpMainName + "/" + MoveType
    if not os.path.exists(OutputPath):
        os.makedirs(OutputPath)
    m.frameCapture.reset()
    m.frameCapture.outputDir = OutputPath
    
    SceneFile = Scene + '.pyscene'
    m.loadScene(SceneParentDir + SceneFile)
    if (Common.Record):
        m.clock.stop()
        m.profiler.enabled = True
        m.profiler.startCapture()
        for i in range(TotalFrame):
            m.renderFrame()
            if (MoveType == 'Dynamic'):
                m.clock.step()
        capture = m.profiler.endCapture()
        m.profiler.enabled = False

        Common.writeJSON(capture, OutputPath + "/" + SceneFile + ".json")
    else:
        input("Not recording. Press Enter to next experiment")

exit()