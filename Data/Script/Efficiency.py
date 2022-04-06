import sys
sys.path.insert(0, 'D:/project/RealTimeRayTracing/Data/Script')

from xml.dom.expatbuilder import ExpatBuilderNS
from falcor import *
import os
import Common
import time

ExpMainName = 'Efficiency'
ExpAlgorithmNames = ['SRGM', 'Tranditional']
ExpAlgorithmGraph = 'Efficiency.py'
ExpSceneNames = ['LightGrid', 'LightDragon', 'LightRobot', 'ObjectGrid', 'ObjectDragon', 'ObjectRobot']
ExpMoveTypes = ['Static', 'Static', 'Static', 'Dynamic', 'Dynamic', 'Dynamic']

SceneParentDir = Common.ScenePath + 'Experiment/Ghosting/'

PrepareFrame = 200
RecordFrame = 2000
m.clock.framerate = 60

def updateParam(ExpName):
    graph = m.activeGraph

    PassSMV = graph.getPass("MS_Visibility")
    PassVis = graph.getPass("STSM_CalculateVisibility")
    PassReuse = graph.getPass("STSM_TemporalReuse")
    PassFilter = graph.getPass("STSM_BilateralFilter")
    PassSRGM = graph.getPass("STSM_ReuseFactorEstimation")

    if ExpName == 'SRGM':
        PassVis.RandomSelection = True
        PassVis.SelectNum = 8
        PassSMV.UseSMV = True
        PassFilter.Enable = True
        PassReuse.AdaptiveAlpha = True
        PassReuse.Alpha = 0.1
        # optimize
        # sm 256 * 256 -1ms
        # PassFilter.Iteration = 1 # -1ms, X (no faster than temporal reuse)
        PassFilter.Iteration = 2
        PassSRGM.ReuseVariation = False # -0.5ms
        PassSRGM.VarOfVarMinFilterKernelSize = 15 # 1/3 -0.5ms
        PassSRGM.VarOfVarMaxFilterKernelSize = 15 # 2/3 -0.5ms
        PassSRGM.VarOfVarTentFilterKernelSize = 15 # 3/3 -0.5ms
    elif ExpName == 'Tranditional':
        PassVis.RandomSelection = False
        PassVis.SelectNum = 16
        PassReuse.AdaptiveAlpha = False
        PassReuse.Alpha = 0.1
        PassSMV.UseSMV = False
        PassFilter.Enable = False

if m.activeGraph:
    m.removeGraph(m.activeGraph)
m.script(Common.GraphPath + ExpAlgorithmGraph) # load graph of algorithm

for ExpIdx, ExpAlgName in enumerate(ExpAlgorithmNames):
    updateParam(ExpAlgName)
    for i, Scene in enumerate(ExpSceneNames):
        MoveType = ExpMoveTypes[i]
            
        OutputPath = Common.OutDir + ExpMainName + "/" + ExpAlgName + "/" + MoveType
        if not os.path.exists(OutputPath):
            os.makedirs(OutputPath)
        m.frameCapture.reset()
        m.frameCapture.outputDir = OutputPath
        
        SceneFile = Scene + '.pyscene'
        m.loadScene(SceneParentDir + SceneFile)

        m.clock.stop()
        for i in range(PrepareFrame):
            m.renderFrame()
            if (MoveType == 'Dynamic'):
                m.clock.step()
        
        if (Common.Record):
            m.profiler.enabled = True
            m.profiler.startCapture()
            for i in range(RecordFrame):
                m.renderFrame()
                if (MoveType == 'Dynamic'):
                    m.clock.step()
            capture = m.profiler.endCapture()
            m.profiler.enabled = False

            capture['ExpName'] = ExpAlgName
            Common.writeJSON(capture, OutputPath + "/" + Scene + ".json")
        else:
            input("Not recording. Press Enter to next experiment")

exit()