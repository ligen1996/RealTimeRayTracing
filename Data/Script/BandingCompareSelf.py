import sys
sys.path.insert(0, 'D:/project/RealTimeRayTracing/Data/Script')

from xml.dom.expatbuilder import ExpatBuilderNS
from falcor import *
import os
import Common
import time

# random: enable random
# Tranditional: disable random

ExpMainName = 'BandingCompareSelf'
ExpAlgorithmNames = ['Random', 'Banding', 'GroundTruth']
ExpAlgorithmGraphs = ['Ghosting-Object.py', 'Ghosting-Object.py', 'GroundTruth.py']
ExpSceneNames = ['GridObserve', 'DragonObserve', 'ArcadeObserve']

SceneParentDir = Common.ScenePath + 'Experiment/' + ExpMainName + '/'

TotalFrame = 100
FramesToCapture = range(60, 70)
m.clock.framerate = 60

def updateParam(ExpName):
    if ExpName == 'GroundTruth':
        return

    graph = m.activeGraph
    PassVis = graph.getPass("STSM_CalculateVisibility")
    if ExpName == 'SRGM':
        PassVis.RandomSelection = True
        PassVis.SelectNum = 8
    elif ExpName == 'TA':
        PassVis.RandomSelection = False
        PassVis.SelectNum = 16

for ExpIdx, ExpAlgName in enumerate(ExpAlgorithmNames):
    GraphName = ExpAlgorithmGraphs[ExpIdx]
    if m.activeGraph:
        m.removeGraph(m.activeGraph)
    m.script(Common.GraphPath + GraphName) # load graph of algorithm
    updateParam(ExpAlgName)
    for Scene in ExpScenes:
        OutputPath = Common.OutDir + ExpMainName + "/" + Scene + "/" + ExpAlgName
        if not os.path.exists(OutputPath):
            os.makedirs(OutputPath)
        m.frameCapture.reset()
        m.frameCapture.outputDir = OutputPath
        
        SceneFile = Scene + '.pyscene'
        ExpName = ExpMainName + '-' + ExpAlgName
        m.loadScene(SceneParentDir + SceneFile)
        if (Common.Record):
            m.clock.stop()
            for i in range(TotalFrame):
                renderFrame()
                if i in FramesToCapture:
                    # just for ground truth
                    if ExpAlgName == 'GroundTruth':
                        for j in range(0, 500):
                            renderFrame()
                    m.frameCapture.baseFilename = ExpName + f"-{i:04d}"
                    m.frameCapture.capture()
                m.clock.step()
            time.sleep(1)
            if not ExpAlgName == 'GroundTruth':
                Common.keepOnlyFile(OutputPath, ["Result", "TR_Visibility"])
            Common.putIntoFolders(OutputPath)
        else:
            input("Not recording. Press Enter to next experiment")
exit()