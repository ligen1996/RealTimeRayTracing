import sys
sys.path.insert(0, 'D:/project/RealTimeRayTracing/Data/Script')

from falcor import *
import os
import Common
import time

# SRGM_Adaptive: all on
# SRGM_NoAdaptive: all on, but no adaptive
# Tranditional: no srgm adaptive, no random SM, no smv, no space reuse

ExpMainName = 'Irregular'
ExpAlgorithmNames = ['SRGM', 'GroundTruth']
ExpAlgorithmGraphs = ['GraphSRGMFinal.py', 'GroundTruth.py']
ExpSceneNames = ['Grid', 'Dragon', 'Robot']
ExpShapes = ['', 'star.bmp', '2.bmp']

SceneParentDir = Common.ScenePath + 'Experiment/Irregular/'

MaskTextureDir = "../../Data/Scene/Experiment/Irregular/"

KeepList = ["LTC"]

TotalFrame = 100
m.clock.framerate = 60

def changeMask(Shape):
    PassSM = m.activeGraph.getPass("STSM_MultiViewShadowMapRasterize")
    if len(Shape) == 0:
        PassSM.MaskTextureFile = ""
    else:
        PassSM.MaskTextureFile = MaskTextureDir + Shape

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
    if ExpName.find('SRGM') >= 0:
        PassVis.RandomSelection = True
        PassVis.SelectNum = 8
        PassSMV.UseSMV = True
        PassFilter.Enable = True
        if ExpName == 'SRGM_Adaptive':
            PassReuse.AdaptiveAlpha = True
            PassReuse.Alpha = 0.1
        elif ExpName == 'SRGM_NoAdaptive':
            PassReuse.AdaptiveAlpha = False
            PassReuse.Alpha = 0.1
    elif ExpName == 'Tranditional':
        PassVis.RandomSelection = False
        PassVis.SelectNum = 16
        PassReuse.AdaptiveAlpha = False
        PassReuse.Alpha = 0.1
        PassSMV.UseSMV = False
        PassFilter.Enable = False


OutputPath = Common.OutDir + ExpMainName + "/"
if not os.path.exists(OutputPath):
    os.makedirs(OutputPath)

for ExpIdx, ExpAlgName in enumerate(ExpAlgorithmNames):
    GraphName = ExpAlgorithmGraphs[ExpIdx]
    if m.activeGraph:
        m.removeGraph(m.activeGraph)
    m.script(Common.GraphPath + GraphName) # load graph of algorithm
    
    for Scene in ExpSceneNames:
        for Shape in ExpShapes:
            ShapeName = Shape.replace(".bmp", "")
            if len(ShapeName) == "":
                ShapeName = "rect"
            changeMask(Shape)
            updateParam(ExpAlgName)
            m.frameCapture.reset()
            m.frameCapture.outputDir = OutputPath
            
            m.loadScene(SceneParentDir + Scene + '.pyscene')
            if (Common.Record):
                m.clock.stop()
                for i in range(TotalFrame):
                    renderFrame()
                    m.clock.step()
                # just for ground truth
                if ExpAlgName == 'GroundTruth':
                    for j in range(0, 400):
                        renderFrame()
                m.frameCapture.baseFilename = Scene + '-' + ExpAlgName + '-' + ShapeName
                m.frameCapture.capture()
            else:
                input("Not recording. Press Enter to next experiment")

time.sleep(10)

Common.cleanup(OutputPath, KeepList)
exit()