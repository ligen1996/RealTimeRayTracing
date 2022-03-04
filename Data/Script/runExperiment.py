import os

gMogwaiExe = "../../Bin/x64/Debug/Mogwai.exe"
gExpScriptName = ["Ghosting", "Flickering", "BandingCompareSelf", "BandingCompareTranditional"]

def chooseExp():
    expNum = len(gExpScriptName)
    def inputExp():
        print("选择要运行的实验：")
        for i in range(expNum):
            print("  %d: %s" % (i + 1, gExpScriptName[i]))
        print("  0: 退出")
        return input()  
    exp = inputExp()
    while (not exp.isdigit() or int(exp) < 0 or int(exp) > expNum):
        print("输入错误！\n")
        exp = inputExp()
    if exp == 0:
        exit()
    else:
        return int(exp)

exp = chooseExp()
expScriptName = gExpScriptName[exp - 1]

print("------------------开始实验------------------")
cmd = "start /wait /b %s -s\"%s.py\" --width=1920 --height=1080" % (gMogwaiExe, expScriptName)
os.system(cmd)
print("------------------实验完成------------------")
os.system("pause")