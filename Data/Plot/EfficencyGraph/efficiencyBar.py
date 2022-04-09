import Common
import os
import matplotlib.pyplot as plt
from matplotlib.font_manager import FontProperties


gDrawTitle = False
gDrawTotal = True
gFontYaHei = FontProperties(fname="C:/Windows/Fonts/msyh.ttc", size=14)
gFontYaHeiDict = {'family':['Microsoft YaHei']}

gMaps = {
    '/onFrameRender/RenderGraphExe::execute()/gpuTime': 'Total',
    'STSM_MultiViewShadowMapRasterize': 'Shadow Maps', 
    'GBufferRaster': 'GBuffer', 
    'STSM_CalculateVisibility': '可见性计算', 
    'MS_Visibility': '阴影重投影', 
    'STSM_ReuseFactorEstimation': 'SRGM', 
    'STSM_TemporalReuse': '时间复用', 
    'STSM_BilateralFilter': '空间滤波',
    'SkyBox': 'SkyBox',
    'LTCLight': 'LTC',
}

gColorMaps = {
    '/onFrameRender/RenderGraphExe::execute()/gpuTime': Common.ColorSet[6],
    'STSM_MultiViewShadowMapRasterize': Common.ColorSet[0], 
    'GBufferRaster': Common.ColorSet[7], 
    'STSM_CalculateVisibility': Common.ColorSet[1], 
    'MS_Visibility': Common.ColorSet[2], 
    'STSM_ReuseFactorEstimation': Common.ColorSet[3], 
    'STSM_TemporalReuse': Common.ColorSet[4], 
    'STSM_BilateralFilter': Common.ColorSet[5],
    'SkyBox': Common.ColorSet[8],
    'LTCLight': Common.ColorSet[9],
}

def mapName(eventName):
    for key in gMaps:
        if eventName.find(key) >= 0:
            return gMaps[key]
    return eventName

def mapColor(eventName):
    for key in gColorMaps:
        if eventName.find(key) >= 0:
            return gColorMaps[key]
    return eventName

def customDiscardFilter(expName, name):
    if expName == 'SRGM':
        removeKeys = ['GBuffer']
    elif expName == 'Tranditional':
        removeKeys = ['GBuffer', 'SkyBox', 'STSM_ReuseFactorEstimation', 'MS_Visibility', 'STSM_BilateralFilter', 'LTC']
    else:
        return False
    for key in removeKeys:
        if (name.find(key) >= 0):
            return True
    return False

def plotGraphData(graphData, save = False, savePath = ''):
    fig = plt.figure(figsize=(12, 9))
    if gDrawTitle:
        plt.title(graphData.title, fontsize = 24)

    X = range(graphData.getRowNum())
    labels = []
    colors = []
    Y = [] # avg
    for i in X:
        [name, data, stat] = graphData.getRow(i)
        labels.append(mapName(name))
        colors.append(mapColor(name))
        Y.append(stat['mean'])
    X = range(graphData.getRowNum())
    Total = sum(Y)
    print("Total: %.2fms" %  Total)

    plt.xticks(X, labels, fontproperties = gFontYaHei)
    plt.bar(X, Y, width=0.5, color=colors)
    # draw percentage
    for i, x in enumerate(X):
        y = Y[i]
        asd = (y / Total * 100)
        strPercentage = "%.1f%%" % (y / Total * 100)
        strMs = "%.2fms" % y
        strAll = strMs + "\n" + strPercentage
        plt.margins(y=0.15)
        plt.text(x, y+0.02, strAll, ha='center', fontsize=10)
    if gDrawTotal:
        plt.xlabel("Total: %.2fms" %  Total, fontsize=15)

    if save:
        plt.savefig(savePath)
    plt.show()

def plotFile(fileName, save = False, savePath = ''):
    graphData = Common.loadProfilerJson(fileName, True, True, customDiscardFilter)
    plotGraphData(graphData, save, savePath)

# plotFile(Common.getFileName())
# try:
#     plotFile(Common.getFileName())
# except Exception as e:
#     print(e)
#     os.system('pause')

# https://zhuanlan.zhihu.com/p/113657235


AlgNames = ['SRGM', 'Tranditional']
MoveTypes = ['Static', 'Dynamic']
SceneNames = ['Grid', 'Dragon', 'Robot']
ResultDir = 'E:/Out/Efficiency/'
def drawAll():
    for Alg in AlgNames:
        for MoveType in MoveTypes:
            for Scene in SceneNames:
                FilePath = "%s/%s/%s/%s.json" % (ResultDir, Alg, MoveType, Scene)
                OutGraphFilePath = "%s/%s_%s_%s.png" % (ResultDir, Alg, MoveType, Scene)
                plotFile(FilePath, True, OutGraphFilePath)

drawAll()
