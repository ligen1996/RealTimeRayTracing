from xml.dom.expatbuilder import ExpatBuilderNS
from falcor import *
import os
import time
import Common

# turn off reliability, enable/disable adaptive
# merge channel = ddv * 10 green

ExpMainName = 'VisAll'
ExpAlgorithmName = ['VisAdaptive', 'VisNoAdaptive', 'GroundTruth'] 
ExpAlgorithmGraph = ['GraphVisAll.py', 'GraphVisAll.py', 'GroundTruth.py']

ExpIdx = 2
GraphName = ExpAlgorithmGraph[ExpIdx]

# auto iteration all types and scenes
ExpType = ['Object', 'Light'] 
ExpSceneName = ['Grid', 'Dragon', 'Arcade']

SceneSubPath = 'Experiment/Ghosting/'

TotalFrame = 130
FramesToCapture = range(120,130)

m.clock.framerate = 60
m.clock.stop()

SceneNames = []
OutputPaths = []
for Scene in ExpSceneName:
    for Type in ExpType:
        SceneNames.append(Type + Scene + '.pyscene')
        OutputPaths.append("d:/Out/" + ExpMainName + "/" + ExpAlgorithmName[ExpIdx] + "/" + Scene + "/" + Type + "/")

for i in range(len(SceneNames)):
    SceneName = SceneNames[i]
    OutputPath = OutputPaths[i]
    if not os.path.exists(OutputPath):
        os.makedirs(OutputPath)
    m.frameCapture.outputDir = OutputPath
    m.script(Common.GraphPath + GraphName)
    m.loadScene(Common.ScenePath + SceneSubPath + SceneName)
    if (Common.Record):
        for i in range(TotalFrame):
            renderFrame()
            if i in FramesToCapture:
                # just for ground truth
                if ExpIdx == 2:
                    for j in range(0, 500):
                        renderFrame()
                m.frameCapture.baseFilename = ExpMainName + f"-{i:04d}"
                m.frameCapture.capture()
            m.clock.step()
exit()

print("waiting...")
time.sleep(10)
print("delete and copying...")

for OutputPath in OutputPaths:
    if not ExpIdx == 2:
        Common.keepOnlyFile(OutputPath, ["Result", "TR_Visibility", "Variation", "VarOfVar", "BilateralFilter.Debug", "TemporalReuse.Debug", "MergeChannels"])
    Common.putIntoFolders(OutputPath)
