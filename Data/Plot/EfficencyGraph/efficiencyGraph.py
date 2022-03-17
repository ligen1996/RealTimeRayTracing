import Common
import os
import matplotlib.pyplot as plt

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
        plt.plot(xData, data, label=Common.cleanEventName(name))
    legendColor = plt.legend(loc = 'upper center', ncol=min(3, graphData.getRowNum()))
    averageLegendData = ['%.3f' % avg for avg in averageData]
    legendAverage = plt.legend(loc = 'lower center', labels=averageLegendData, title="average",ncol=min(8, graphData.getRowNum()))
    fig.add_artist(legendColor) # first legend will be replaced, so add it back to graph
    plt.margins(x = 0)
    plt.show()

def plotFile(fileName):
    graphData = Common.loadProfilerJson(fileName)
    plotGraphData(graphData)

try:
    plotFile(Common.getFileName())
except Exception as e:
    print(e)
    os.system('pause')