from xml.dom.expatbuilder import ExpatBuilderNS
from falcor import *
import os
import Common
import time

# Filtered: enable space filter
# Tranditional: disable space filter

ExpMainName = 'Convergence'
ExpAlgorithmName = ['Filtered', 'Original', 'GroundTruth']
ExpAlgorithmGraph = ['Ghosting-Object-NOSMV.py','Ghosting-Object-NOSMV.py','GroundTruth.py']
# ExpSceneName = ['GridObserve', 'DragonObserve', 'ArcadeObserve']
# ExpSceneName = ['GridObserve', 'DragonObserve', 'ArcadeObserve', 'DynamicGridObserve', 'DynamicDragonObserve', 'DynamicArcadeObserve']
ExpSceneName = [ 'DynamicGridObserve', 'DynamicDragonObserve', 'DynamicArcadeObserve']
ExpIdx = 2

SceneSubPath = 'Experiment/Convergence/'
ExpName = ExpMainName + '-' + ExpAlgorithmName[ExpIdx]

GraphName = ExpAlgorithmGraph[ExpIdx]
m.script(Common.GraphPath + GraphName)

TotalFrame = 120
FramesToCapture = range(0, 100)
m.clock.framerate = 120

for SceneName in ExpSceneName:
    SceneFile = SceneName + '.pyscene'
    OutputPath = "d:/Out/" + ExpMainName + "/" + SceneName + "/" + ExpAlgorithmName[ExpIdx]
    
    Common.removeDir(OutputPath)
    os.makedirs(OutputPath)

    m.loadScene(Common.ScenePath + SceneSubPath + SceneFile)

    if (Common.Record):
        m.clock.stop()
        m.frameCapture.outputDir = OutputPath

        for i in range(TotalFrame):
            renderFrame()
            if i in FramesToCapture:
                # just for ground truth
                if ExpIdx == 2:
                    for j in range(0, 500):
                        renderFrame()
                m.frameCapture.baseFilename = ExpName + f"-{i:04d}"
                m.frameCapture.capture()
            m.clock.step()
        time.sleep(2)
        if not ExpIdx == 2:
            Common.keepOnlyFile(OutputPath, ["Result", "TR_Visibility"])
        Common.putIntoFolders(OutputPath)

exit()
