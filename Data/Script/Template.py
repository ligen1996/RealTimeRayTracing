from falcor import *

GraphPath = '../../Data/Graph/'
GraphName = 'test.py'
ScenePath = '../../../Data/Scene/'
SceneSubPath = 'Experiment/Ghosting/'
SceneName = 'Camera.pyscene'

m.script(GraphPath+GraphName)
m.loadScene(ScenePath+SceneSubPath+SceneName)

m.clock.framerate = 30

frames = [520, 521, 522, 523]

m.clock.stop()
m.frameCapture.outputDir = "d:/Out"

for i in range(600):
    renderFrame()
    if i in frames:
        m.frameCapture.baseFilename = f"Mogwai-{i:04d}"
        m.frameCapture.capture()
    m.clock.step()
exit()

