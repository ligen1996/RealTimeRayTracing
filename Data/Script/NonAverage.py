import sys
sys.path.insert(0, 'D:/project/RealTimeRayTracing/Data/Script')

from falcor import *
import os
import Common
import time

ExpMainName = 'NonAverage'
ExpAlgorithmNames = ['SRGM', 'GroundTruth']
ExpAlgorithmGraphs = ['GraphSRGMFinal.py', 'GroundTruth.py']
ExpSceneNames = ['Grid', 'Dragon', 'Robot']
ExpTextures = ['', 'GradientQuantified', 'Halo', 'BluredBunny', 'Cell']

SceneParentDir = Common.ScenePath + 'Experiment/' + ExpMainName + "/"

TextureListDir = "../../Data/Texture/"

KeepList = ["LTC"]

TotalFrame = 100
m.clock.framerate = 60

def changeTexture(Texture):
    Dir = TextureListDir + Texture + "/"
    PassSM = m.activeGraph.getPass("STSM_MultiViewShadowMapRasterize")
    PassLTC = m.activeGraph.getPass("LTCLight")
    if len(Texture) == 0:
        PassSM.setLightTexture("")
    else:
        PassSM.setLightTexture(Dir + "0.png")
        PassLTC.generateLightColorTex(Dir)

def updateParam(ExpName):
    graph = m.activeGraph

    if ExpName == 'GroundTruth':
        PassVis = graph.getPass("STSM_CalculateVisibility")
        PassVis.RandomSelection = False
        PassVis.SelectNum = 16
        return


OutputPath = Common.OutDir + ExpMainName + "/"
if not os.path.exists(OutputPath):
    os.makedirs(OutputPath)

for ExpIdx, ExpAlgName in enumerate(ExpAlgorithmNames):
    GraphName = ExpAlgorithmGraphs[ExpIdx]
    if m.activeGraph:
        m.removeGraph(m.activeGraph)
    m.script(Common.GraphPath + GraphName) # load graph of algorithm
    
    for Scene in ExpSceneNames:
        for Texture in ExpTextures:
            TextureName = Texture
            if len(Texture) == "":
                TextureName = "Rectangle"
            changeTexture(Texture)
            updateParam(ExpAlgName)
            m.frameCapture.reset()
            m.frameCapture.outputDir = OutputPath
            
            m.loadScene(SceneParentDir + Scene + '.pyscene')
            if (Common.Record):
                m.clock.stop()
                for i in range(TotalFrame):
                    renderFrame()
                    m.clock.step()
                # just for ground truth
                if ExpAlgName == 'GroundTruth':
                    for j in range(0, 400):
                        renderFrame()
                m.frameCapture.baseFilename = Scene + '-' + ExpAlgName + '-' + TextureName
                m.frameCapture.capture()
            else:
                input("Not recording. Press Enter to next experiment")

time.sleep(10)

Common.cleanup(OutputPath, KeepList)
exit()