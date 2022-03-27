#include "MS_Shadow.h"
#include "MS_Visibility.h"

static const char kDescShadow[] = "Shadow Map";
static const char kDescVisibility[] = "Visibility Map";

extern "C" __declspec(dllexport) const char* getProjDir()
{
    return PROJECT_DIR;
}

extern "C" __declspec(dllexport) void getPasses(Falcor::RenderPassLibrary & lib)
{
    lib.registerClass("MS_Shadow", kDescShadow, MS_Shadow::create);
    lib.registerClass("MS_Visibility", kDescVisibility, MS_Visibility::create);
    ScriptBindings::registerBinding(MS_Visibility::registerScriptBindings);
}
