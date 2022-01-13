#include "BilateralFilter.h"

namespace
{
    const char kDesc[] = "Bilateral Filter pass";

    // input
    const std::string kColor = "Color";
    const std::string kNormal = "Normal";
    const std::string kDepth = "Depth";

    // output
    const std::string kResult = "Result";
   
    // shader file path
    const std::string kPassfile = "RenderPasses/SpatioTemporalSM/BilateralFilter.ps.slang";
}

Gui::DropdownList STSM_BilateralFilter::mDirectionList =
{
    Gui::DropdownValue{ int(EFilterDirection::X), toString(EFilterDirection::X) },
    Gui::DropdownValue{ int(EFilterDirection::Y), toString(EFilterDirection::Y) },
    Gui::DropdownValue{ int(EFilterDirection::BOTH), toString(EFilterDirection::BOTH) }
};

STSM_BilateralFilter::STSM_BilateralFilter()
{
    createPassResouces();
}

STSM_BilateralFilter::SharedPtr STSM_BilateralFilter::create(RenderContext* pRenderContext, const Dictionary& dict)
{
    return SharedPtr(new STSM_BilateralFilter());
}

std::string STSM_BilateralFilter::getDesc() { return kDesc; }

Dictionary STSM_BilateralFilter::getScriptingDictionary()
{
    return Dictionary();
}

RenderPassReflection STSM_BilateralFilter::reflect(const CompileData& compileData)
{
    // Define the required resources here
    RenderPassReflection reflector;
    reflector.addInput(kColor, "Color");
    reflector.addInput(kNormal, "Normal");
    reflector.addInput(kDepth, "Depth");
    reflector.addOutput(kResult, "Result").bindFlags(Resource::BindFlags::RenderTarget | Resource::BindFlags::ShaderResource).format(ResourceFormat::RGBA32Float).texture2D(0, 0);
    return reflector;
}

void STSM_BilateralFilter::compile(RenderContext* pContext, const CompileData& compileData)
{
}

void STSM_BilateralFilter::execute(RenderContext* pRenderContext, const RenderData& renderData)
{
    const auto& pColor = renderData[kColor]->asTexture();
    const auto& pNormal = renderData[kNormal]->asTexture();
    const auto& pDepth = renderData[kDepth]->asTexture();
    const auto& pResult = renderData[kResult]->asTexture();

    mVFilterPass.mpFbo->attachColorTarget(pResult, 0);
    mVFilterPass.mpPass["PerFrameCB"]["gEnable"] = mVContronls.Enable;
    mVFilterPass.mpPass["PerFrameCB"]["gDirection"] = (int)mVContronls.Direction;
    mVFilterPass.mpPass["PerFrameCB"]["gSigmaColor"] = mVContronls.SigmaColor;
    mVFilterPass.mpPass["PerFrameCB"]["gSigmaNormal"] = mVContronls.SigmaNormal;
    mVFilterPass.mpPass["PerFrameCB"]["gSigmaDepth"] = mVContronls.SigmaDepth;
    mVFilterPass.mpPass["PerFrameCB"]["gKernelSize"] = mVContronls.KernelSize;
    mVFilterPass.mpPass["gTexColor"] = pColor;
    mVFilterPass.mpPass["gTexNormal"] = pNormal;
    mVFilterPass.mpPass["gTexDepth"] = pDepth;
    mVFilterPass.mpPass->execute(pRenderContext, mVFilterPass.mpFbo);
}

void STSM_BilateralFilter::renderUI(Gui::Widgets& widget)
{
    widget.checkbox("Enable", mVContronls.Enable);
    if (mVContronls.Enable)
    {
        uint Index = (uint)mVContronls.Direction;
        widget.dropdown("Direction", mDirectionList, Index);
        mVContronls.Direction = (EFilterDirection)Index;
        widget.var("Sigma Color", mVContronls.SigmaColor, 1.0f, 50.0f, 0.1f);
        widget.var("Sigma Normal", mVContronls.SigmaNormal, 1.0f, 50.0f, 0.1f);
        widget.var("Sigma Depth", mVContronls.SigmaDepth, 1.0f, 50.0f, 0.1f);
        widget.var("Kernel Size", mVContronls.KernelSize, 3u, 101u, 2u);
    }
}

std::string STSM_BilateralFilter::toString(EFilterDirection vType)
{
    switch (vType)
    {
    case EFilterDirection::X: return "X";
    case EFilterDirection::Y: return "Y";
    case EFilterDirection::BOTH: return "BOTH";
    default: should_not_get_here(); return "";
    }
}

void STSM_BilateralFilter::createPassResouces()
{
    mVFilterPass.mpPass = FullScreenPass::create(kPassfile);
    mVFilterPass.mpFbo = Fbo::create();
}
