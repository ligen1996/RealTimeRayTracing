#include "TemporalFilter.h"
#include "RTBilateralFilter.h"


static const char DescReuseFactorEstimation[] = "Reuse Factor Estimation";
static const char DescReuse[] = "RT Temporal Reuse";
static const char DescBilateralFilter[] = "Bilateral Filter";

extern "C" __declspec(dllexport) const char* getProjDir()
{
    return PROJECT_DIR;
}

extern "C" __declspec(dllexport) void getPasses(Falcor::RenderPassLibrary & lib)
{
    lib.registerClass("TemporalFilter", DescReuse, TemporalFilter::create);
    lib.registerClass("BilateralFilter", DescBilateralFilter, BilateralFilter::create);

    ScriptBindings::registerBinding(TemporalFilter::registerScriptBindings);
    ScriptBindings::registerBinding(BilateralFilter::registerScriptBindings);
}
