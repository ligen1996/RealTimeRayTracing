import os
import json
import re
import sys
import matplotlib.pyplot as plt

class GraphData:
    def __init__(self, title, pointNum):
        self.init(title, pointNum)
    
    def init(self, title, pointNum):
        self.title = title
        self.pointNum = pointNum
        self.names = []
        self.datas = []
    
    def addRow(self, name, data):
        if (len(data) != self.pointNum):
            raise
        self.names.append(name)
        self.datas.append(data)

    def getRowNum(self):
        return len(self.names)

    def getRow(self, index):
        if (index < 0 or index >= self.getRowNum()):
            raise
        return [self.names[index], self.datas[index]]

def filterEvent(eventName):
    # filter by key
    removeKeys = ['cpuTime', 'present', 'animate', 'renderUI']
    for key in removeKeys:
        if (eventName.find(key) >= 0):
            return False

    # FIXME: this remove all sub sequence, might be wrong
    cleanedName = cleanEventName(eventName)
    if (cleanedName.find("/") >= 0):
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

def loadProfilerJson(fileName):
    jsonData = json.loads(loadFileData(fileName))
    pointNum = int(jsonData['frameCount'])
    # gpuTimeData = jsonData['events']['/present/gpuTime']['records'] # in ms

    graphData = GraphData(fileName[max(fileName.rfind("\\") + 1, 0):], pointNum)
    # graphData.addRow("Full GPU", gpuTimeData)
    for eventName in jsonData['events']:
        if not filterEvent(eventName):
            continue
        graphData.addRow(eventName, jsonData['events'][eventName]['records'])

    return graphData

def plotGraphData(graphData):
    fig = plt.figure()
    plt.title(graphData.title, fontsize = 24)
    plt.xlabel("Frame", fontsize = 14)
    plt.ylabel("ms", fontsize = 14)
    xData = range(1, graphData.pointNum + 1)
    averageData = []
    for i in range(graphData.getRowNum()):
        [name, data] = graphData.getRow(i)
        avg = sum(data) / len(data)
        averageData.append(avg)
        plt.plot(xData, data, label=cleanEventName(name))
    legendColor = plt.legend(loc = 'upper center', ncol=min(3, graphData.getRowNum()))
    averageLegendData = ['%.3f' % avg for avg in averageData]
    legendAverage = plt.legend(loc = 'lower center', labels=averageLegendData, title="average",ncol=min(8, graphData.getRowNum()))
    fig.add_artist(legendColor) # first legend will be replaced, so add it back to graph
    plt.margins(x = 0)
    plt.show()

def plotFile(fileName):
    graphData = loadProfilerJson(fileName)
    plotGraphData(graphData)

def getFileName():
    if len(sys.argv) == 1:
        return input("Profile file name: ")
    else:
        return sys.argv[1]


try:
    plotFile(getFileName())
except Exception as e:
    print(e)
    os.system('pause')