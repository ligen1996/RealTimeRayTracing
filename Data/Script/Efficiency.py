from xml.dom.expatbuilder import ExpatBuilderNS
from falcor import *
import os
import json
import Common

ExpMainName = 'Efficiency'
ExpAlgorithmName = ['Static', 'Dynamic', 'Mixed']
ExpAlgorithmScene = ['StaticTube', 'DynamicTube', 'MixedTube']
ExpAlgorithmGraph = ['Ghosting-Object.py','Ghosting-Object-NoSMV.py']
GraphName = ExpAlgorithmGraph[0]

SceneSubPath = 'Experiment/' + ExpMainName + '/'
OutputPath = "d:/Out/" + ExpMainName
if not os.path.exists(OutputPath):
    os.makedirs(OutputPath)

TotalFrame = 2000

for ExpIdx in range(len(ExpAlgorithmName)):
    SceneName = ExpAlgorithmScene[ExpIdx] + '.pyscene'
    ExpName = ExpMainName + '-' + ExpAlgorithmName[ExpIdx]

    m.script(Common.GraphPath + GraphName)
    m.loadScene(Common.ScenePath + SceneSubPath + SceneName)

    if (Common.Record):
        m.profiler.enabled = True
        m.profiler.startCapture()
        for i in range(TotalFrame):
            m.renderFrame()
        capture = m.profiler.endCapture()
        m.profiler.enabled = False

        Common.writeJSON(capture, OutputPath + "/" + ExpName + ".json")
    else:
        if ExpIdx == 0:
            print("Not Recording. So the rest graphs wont be loaded.")
        else:
            continue

exit()