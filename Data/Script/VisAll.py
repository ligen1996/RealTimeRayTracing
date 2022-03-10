from xml.dom.expatbuilder import ExpatBuilderNS
from falcor import *
import os
import time
import Common

# turn off reliability, no adaptive
# merge channel green * 5

ExpMainName = 'VisAll'
ExpAlgorithmName = ['Vis', 'GroundTrush', 'GroundTruth']
ExpAlgorithmGraph = ['GraphVisAll.py','GroundTruth.py']

ExpIdx = 1
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
        OutputPaths.append("d:/Out/" + ExpMainName + "/" + Scene + "/" + Type + "/")

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
                if ExpIdx == 1:
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
    if not ExpIdx == 1:
        Common.keepOnlyFile(OutputPath, ["Result", "TR_Visibility", "Variation", "VarOfVar", "BilateralFilter.Debug", "TemporalReuse.Debug", "MergeChannels"])
    Common.putIntoFolders(OutputPath)

if ExpIdx == 0:
    print("SRGM+Alpha+滤波核")
else:
    print("Ground Truth")