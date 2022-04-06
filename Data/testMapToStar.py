import matplotlib.pyplot as plt
import math

def getRectLen(x, y):
    maxComponent = max(abs(x), abs(y))
    x = x / maxComponent
    y = y / maxComponent
    return math.sqrt(x * x + y * y)

def getStarLen(x, y):
    r = math.atan2(y,x) + math.pi
    r = r + math.pi * 2 * (3 / 20) # shift to start
    r = r % (math.pi * 2 * 0.2) # mod to [0, 2/5pi]
    r = r - math.pi * 2 * 0.1 # shift to [-1/5pi, 1/5pi]
    r = abs(r) # to [0, 1/5pi]
    alpha = math.pi * 0.1 # half of peek angle of star
    starLen = math.sin(alpha) / math.sin(math.pi - alpha - r)# <= a/sinα=b/sinβ
    return starLen

def normalize(x, y):
    length = getRectLen(x, y)
    return [x / length, y / length]

def correct(x, y): # to [-1, 1]
    # original star: x in [-cos(pi/10), cos(pi/10)], y in [-cos(pi/5), 1]
    minX = -math.cos(math.pi / 10)
    maxX = -minX
    minY = -math.cos(math.pi / 5)
    maxY = 1
    return [(2 * x - (maxX + minX)) / (maxX - minX), (2 * y - (maxY + minY)) / (maxY - minY)]

fig = plt.figure(figsize=(9, 9))

X = []
Y = []

Size = 100
halfMargin = 0.5 / Size 
# for i in range(Size):
#     Tick = i * 2 / Size - 1 + halfMargin
#     X.append(Tick)
#     Y.append(-1)
#     X.append(Tick)
#     Y.append(1)
#     X.append(-1)
#     Y.append(Tick)
#     X.append(1)
#     Y.append(Tick)

for i in range(Size):
    for k in range(Size):
        Tick = i * 2 / Size - 1
        X.append(i * 2 / Size - 1 + halfMargin)
        Y.append(k * 2 / Size - 1 + halfMargin)

plt.scatter(X, Y, color="red")

NewX = []
NewY = []

for i in range(len(X)):
    x = X[i]
    y = Y[i]
    starLen = getStarLen(x, y)
    scale = starLen

    [nx, ny] = normalize(x, y)
    nx = nx * scale
    ny = ny * scale
    #star = correct(nx, ny)
    NewX.append(nx)
    NewY.append(ny)

plt.scatter(NewX, NewY, color="green")


plt.show()