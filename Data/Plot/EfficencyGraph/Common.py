import json
import re
import sys

class GraphData:
    def __init__(self, title, pointNum):
        self.init(title, pointNum)
    
    def init(self, title, pointNum):
        self.title = title
        self.pointNum = pointNum
        self.names = []
        self.datas = []
        self.stats = []
    
    def addRow(self, name, data, stat):
        if (len(data) != self.pointNum):
            raise
        self.names.append(name)
        self.datas.append(data)
        self.stats.append(stat)

    def getRowNum(self):
        return len(self.names)

    def getRow(self, index):
        if (index < 0 or index >= self.getRowNum()):
            raise
        return [self.names[index], self.datas[index], self.stats[index]]

def filterEvent(eventName, vFilterTotal = False):
    # keep only graph execute
    if eventName.find('RenderGraphExe::execute()') == -1:
        return False
        
    # filter by key
    removeKeys = ['cpuTime', 'present', 'animate', 'renderUI']
    for key in removeKeys:
        if (eventName.find(key) >= 0):
            return False

    cleanedName = cleanEventName(eventName)
    if (vFilterTotal and cleanedName == ''):
        return False

    # FIXME: this remove all sub sequence, might be wrong
    if (eventName.count("/") >= 5):
        return False

    return True

def cleanEventName(eventName):
    eventName = eventName.replace("/gpuTime", "")
    eventName = eventName.replace("/onFrameRender", "")
    eventName = eventName.replace("/RenderGraphExe::execute()", "")
    eventName = re.sub(r"^/", "", eventName)
    return eventName

def loadFileData(fileName):
    return open(fileName).read()

def loadProfilerJson(fileName, vFilterEvent = True, vFilterTotal = False):
    jsonData = json.loads(loadFileData(fileName))
    pointNum = int(jsonData['frameCount'])
    # gpuTimeData = jsonData['events']['/present/gpuTime']['records'] # in ms

    graphData = GraphData(fileName[max(fileName.rfind("\\") + 1, 0):], pointNum)
    # graphData.addRow("Full GPU", gpuTimeData)
    for eventName in jsonData['events']:
        if vFilterEvent and not filterEvent(eventName, vFilterTotal):
            continue
        graphData.addRow(eventName, jsonData['events'][eventName]['records'], jsonData['events'][eventName]['stats'])

    return graphData

def getFileName():
    if len(sys.argv) == 1:
        return input("Profile file name: ")
    else:
        return sys.argv[1]

ColorSet = ['#1f77b4', '#ff7f0e', '#2ca02c', '#d62728', '#9467bd', '#8c564b', '#e377c2', '#7f7f7f', '#bcbd22', '#17becf']