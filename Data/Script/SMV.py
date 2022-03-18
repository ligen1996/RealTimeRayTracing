import sys
sys.path.insert(0, 'D:/project/RealTimeRayTracing/Data/Script')

from xml.dom.expatbuilder import ExpatBuilderNS
from falcor import *
import os
import time
import Common

# no adapative, alpha = 0.1
# SMV: use shadow motion vector 
# NOSMV: normal motion vector

ExpMainName = 'SMV'
ExpAlgorithmNames = ['SMV','NoSMV','GroundTruth']
ExpAlgorithmGraphs = ['Ghosting-Object.py', 'Ghosting-Object.py', 'GroundTruth.py']
ExpMoveTypes = ['Object', 'Light']
ExpSceneNames = ['Grid', 'Dragon', 'Arcade']

SceneParentDir = Common.ScenePath + 'Experiment/Ghosting/'

TotalFrame = 100
FramesToCapture = range(60, 70)
m.clock.framerate = 60

def updateParam(ExpName):
    if ExpName == 'GroundTruth':
        return

    graph = m.activeGraph
    PassSMV = graph.getPass("MS_Visibility")
    PassReuse = graph.getPass("TemporalReuse")
    PassReuse.AdaptiveAlpha = False
    PassReuse.Alpha = 0.1
    if ExpName == 'SMV':
        PassSMV.UseSMV = True
    elif ExpName == 'NoSMV':
        PassSMV.UseSMV = False

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
                time.sleep(2)
                if not ExpAlgName == 'GroundTruth':
                    Common.keepOnlyFile(OutputPath, ["Result", "TR_Visibility"])
                Common.putIntoFolders(OutputPath)
            else:
                input("Not recording. Press Enter to next experiment")
exit()     