import Common
import os
import matplotlib.pyplot as plt

gMaps = {
    'STSM_MultiViewShadowMapRasterize': 'SM', 
    'GBufferRaster': 'GBuffer', 
    'STSM_CalculateVisibility': 'Vis', 
    'MS_Visibility': 'SMV', 
    'STSM_ReuseFactorEstimation': 'SRGM', 
    'STSM_TemporalReuse': 'Reuse', 
    'STSM_BilateralFilter': 'Filter'
}

def mapName(eventName):
    for key in gMaps:
        if eventName.find(key) >= 0:
            return gMaps[key]
    return eventName

def plotGraphData(graphData):
    fig = plt.figure()
    plt.title(graphData.title, fontsize = 24)

    averageData = []
    labels = []
    for i in range(graphData.getRowNum()):
        [name, data, stat] = graphData.getRow(i)
        labels.append(mapName(name))
        averageData.append(stat['mean'])
    plt.pie(x = averageData, labels = labels)
    plt.show()

def plotFile(fileName):
    graphData = Common.loadProfilerJson(fileName, True, True)
    plotGraphData(graphData)

try:
    plotFile(Common.getFileName())
except Exception as e:
    print(e)
    os.system('pause')