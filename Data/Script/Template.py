from falcor import *

GraphPath = '../../Data/Graph/'
ScenePath = '../../../Data/Scene/'
SceneSubPath = 'Experiment/Ghosting/'

GraphName = 'test.py'
SceneName = 'Camera.pyscene'

OutputDir = "d:/Out"
ResultPrefix = 'Ghosting-Camera'
TotalFrame = 300
FramesToCapture = range(200,210)

m.script(GraphPath+GraphName)
m.loadScene(ScenePath+SceneSubPath+SceneName)

m.clock.framerate = 30

m.clock.stop()
m.frameCapture.outputDir = OutputDir

for i in range(TotalFrame):
    renderFrame()
    if i in FramesToCapture:
        m.frameCapture.baseFilename = ResultPrefix + f"-{i:04d}"
        m.frameCapture.capture()
    m.clock.step()
exit()

