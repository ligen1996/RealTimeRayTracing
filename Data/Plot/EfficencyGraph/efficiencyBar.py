import Common
import os
import matplotlib.pyplot as plt

gMaps = {
    '/onFrameRender/RenderGraphExe::execute()/gpuTime': 'Total',
    'STSM_MultiViewShadowMapRasterize': 'SM', 
    'GBufferRaster': 'GBuffer', 
    'STSM_CalculateVisibility': 'Vis', 
    'MS_Visibility': 'SMV', 
    'STSM_ReuseFactorEstimation': 'SRGM', 
    'STSM_TemporalReuse': 'Reuse', 
    'STSM_BilateralFilter': 'Filter',
    'SkyBox': 'SkyBox',
    'LTCLight': 'LTC',
}

def mapName(eventName):
    for key in gMaps:
        if eventName.find(key) >= 0:
            return gMaps[key]
    return eventName

def plotGraphData(graphData):
    fig = plt.figure()
    plt.title(graphData.title, fontsize = 24)

    X = range(graphData.getRowNum())
    labels = []
    Y = [] # avg
    for i in X:
        [name, data, stat] = graphData.getRow(i)
        labels.append(mapName(name))
        Y.append(stat['mean'])
    X = range(graphData.getRowNum())
    SumY = sum(Y)

    plt.xticks(X, labels)
    plt.bar(X, Y, width=0.5, color=Common.ColorSet)
    # draw percentage
    for i, x in enumerate(X):
        y = Y[i]
        asd = (y / SumY * 100)
        strPercentage = "%.1f%%" % (y / SumY * 100)
        strMs = "%.2fms" % y
        strAll = strMs + "\n" + strPercentage
        plt.margins(y=0.15)
        plt.text(x, y+0.02, strAll, ha='center', fontsize=10)
    plt.show()

def plotFile(fileName):
    graphData = Common.loadProfilerJson(fileName, True, True)
    plotGraphData(graphData)

plotFile(Common.getFileName())
# try:
#     plotFile(Common.getFileName())
# except Exception as e:
#     print(e)
#     os.system('pause')

# https://zhuanlan.zhihu.com/p/113657235