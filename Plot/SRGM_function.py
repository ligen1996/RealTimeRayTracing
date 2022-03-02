from matplotlib import pyplot as plt
import numpy as np
import math
from mpl_toolkits.mplot3d import Axes3D

def getZ(X, Y, alpha, beta):
    # current, dv=ddv=1->no reuse
    amb = alpha - beta
    a = amb
    b = -2. * amb
    c = amb
    d = beta

    # dv=ddv=1-> reuse
    # b = -amb
    # return np.power(a * X * X + b * X * Y + c * Y * Y + d, 2)
    return a * X * X + b * X * Y + c * Y * Y + d

figure = plt.figure()
axes = Axes3D(figure)

X = np.arange(0,1,0.02)
Y = np.arange(0,1,0.02)#前两个参数为自变量取值范围

X,Y = np.meshgrid(X,Y)

Z = getZ(X, Y, 0.1, 1.0)
axes.plot_surface(X,Y,Z,cmap='rainbow')
plt.show()