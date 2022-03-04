from xml.dom.expatbuilder import ExpatBuilderNS
from falcor import *

ExpMainName = 'Ghosting'
ExpSubName = 'Object'
ExpAlgorithmName = ['SRGM','TA','GroundTruth']
ExpAlgorithmGraph = ['Ghosting-Object.py','Ghosting-Object-NoSMV.py','GroundTruth.py']
ExpIdx = 0

GraphPath = '../../Data/Graph/'
ScenePath = '../../../Data/Scene/'
SceneSubPath = 'Experiment/' + ExpMainName + '/'

GraphName = ExpAlgorithmGraph[0]
SceneName = ExpSubName+'.pyscene'

OutputPath = "d:/Out/" + ExpMainName + "/" + ExpSubName

ExpName = ExpMainName + '-' + ExpSubName + '-' + ExpAlgorithmName[ExpIdx]
TotalFrame = 200
FramesToCapture = range(60,70)

m.script(GraphPath+GraphName)
m.loadScene(ScenePath+SceneSubPath+SceneName)

m.clock.framerate = 120

# m.clock.stop()
# m.frameCapture.outputDir = OutputPath

# for i in range(TotalFrame):
#     renderFrame()
#     if i in FramesToCapture:
#         # just for ground truth
#         if ExpIdx == 2 :
#             for j in range(0,100):
#                 renderFrame()
#         m.frameCapture.baseFilename = ExpName + f"-{i:04d}"
#         m.frameCapture.capture()
#     m.clock.step()
# exit()