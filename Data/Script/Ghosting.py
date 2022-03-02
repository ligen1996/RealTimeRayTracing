from xml.dom.expatbuilder import ExpatBuilderNS
from falcor import *

GraphPath = '../../Data/Graph/'
ScenePath = '../../../Data/Scene/'
SceneSubPath = 'Experiment/Ghosting/'

ExpMainName = 'Ghosting'
ExpSubName = 'Object'
ExpAlgorithmName = ['SRGM','TA','GroundTruth']
ExpIdx = 0

GraphName = 'GraphSTSM_MS_FINAL.py'
SceneName = ExpSubName+'.pyscene'

OutputPath = "d:/Out/Ghosting/" + ExpSubName

ExpName = ExpMainName + '-' + ExpSubName + '-' + ExpAlgorithmName[ExpIdx]
TotalFrame = 300
FramesToCapture = range(200,210)

m.script(GraphPath+GraphName)
m.loadScene(ScenePath+SceneSubPath+SceneName)

m.clock.framerate = 30

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