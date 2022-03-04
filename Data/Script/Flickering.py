from xml.dom.expatbuilder import ExpatBuilderNS
from falcor import *

ExpMainName = 'Flickering'
ExpSubName = ['Dynamic','Static']
ExpObjName = ['Dragon','Grid','Cube']
ExpIdx = 1
ExpObjId = 0
RepeatNum = 3
if(ExpIdx == 1):
    RepeatNum = 1

GraphPath = '../../Data/Graph/'
ScenePath = '../../../Data/Scene/'
SceneSubPath = 'Experiment/' + ExpMainName + '/'

GraphName = 'Ghosting-Object-NoSMV.py'
SceneName = ExpObjName[ExpObjId]+'.pyscene'

OutputPath = "d:/Out/" + ExpMainName + "/" + ExpSubName[ExpIdx]

ExpName = ExpMainName + '-' + ExpSubName[ExpIdx]+ExpObjName[ExpObjId]
TotalFrame = 200
FramesToCapture = range(60,70)

m.script(GraphPath+GraphName)
m.loadScene(ScenePath+SceneSubPath+SceneName)

m.clock.framerate = 120

m.frameCapture.outputDir = OutputPath

for j in range(RepeatNum):
    m.clock.stop()
    for i in range(TotalFrame):
        renderFrame()
        if i in FramesToCapture:
            m.frameCapture.baseFilename = ExpName + f"-{i:04d}" + '-' + f"{j:d}"
            m.frameCapture.capture()
        if(ExpIdx == 0):
            m.clock.step()

exit()