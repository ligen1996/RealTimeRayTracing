import sys
sys.path.insert(0, 'D:/project/RealTimeRayTracing/Data/Script')
from falcor import *
import Common
m.frameCapture.reset()
m.frameCapture.outputDir = Common.OutDir
m.frameCapture.baseFilename = ""
m.frameCapture.capture()