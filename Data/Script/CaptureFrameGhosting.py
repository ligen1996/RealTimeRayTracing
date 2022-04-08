import sys
sys.path.insert(0, 'D:/project/RealTimeRayTracing/Data/Script')
import os
from falcor import *
import Common

OutDir = Common.OutDir + "/VisGhosting"
if not os.path.exists(OutDir):
    os.makedirs(OutDir)

m.frameCapture.reset()
m.frameCapture.outputDir = OutDir

fps = 40

m.clock.stop()
m.clock.framerate = fps
for i in range(fps * 3 - 2):
    m.renderFrame()
    m.clock.step()

m.frameCapture.baseFilename = "Ghosting"
m.frameCapture.capture()

graph = m.activeGraph
PassReuse = graph.getPass("STSM_TemporalReuse")
PassReuse.Alpha = 0.01
for i in range(400):
    m.renderFrame()

m.frameCapture.baseFilename = "GroundTruth"
m.frameCapture.capture()
