#include "MS_Shadow.h"

static const char kDescShadow[] = "Shadow Map";

extern "C" __declspec(dllexport) const char* getProjDir()
{
    return PROJECT_DIR;
}

extern "C" __declspec(dllexport) void getPasses(Falcor::RenderPassLibrary & lib)
{
    lib.registerClass("MS_Shadow", kDescShadow, MS_Shadow::create);
}
