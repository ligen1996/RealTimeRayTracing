#include "BilateralFilter.h"

namespace
{
    const char kDesc[] = "Bilateral Filter pass";

    // input
    const std::string kInput = "Input";

    // output
    const std::string kResult = "Result";
   
    // shader file path
    const std::string kPassfile = "RenderPasses/SpatioTemporalSM/BilateralFilter.ps.slang";
}

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
    reflector.addInput(kInput, "Input");
    reflector.addOutput(kResult, "Result").bindFlags(Resource::BindFlags::RenderTarget | Resource::BindFlags::ShaderResource).format(ResourceFormat::RGBA32Float).texture2D(0, 0);
    return reflector;
}

void STSM_BilateralFilter::compile(RenderContext* pContext, const CompileData& compileData)
{
}

void STSM_BilateralFilter::execute(RenderContext* pRenderContext, const RenderData& renderData)
{
    const auto& pInput = renderData[kInput]->asTexture();
    const auto& pResult = renderData[kResult]->asTexture();

    mVFilterPass.mpFbo->attachColorTarget(pResult, 0);
    mVFilterPass.mpPass["PerFrameCB"]["gSigma"] = mVContronls.Sigma;
    mVFilterPass.mpPass["PerFrameCB"]["gBSigma"] = mVContronls.BSigma;
    mVFilterPass.mpPass["PerFrameCB"]["gMSize"] = mVContronls.MSize;
    mVFilterPass.mpPass["gTexInput"] = pInput;
    mVFilterPass.mpPass->execute(pRenderContext, mVFilterPass.mpFbo);
}

void STSM_BilateralFilter::renderUI(Gui::Widgets& widget)
{
    widget.var("Sigma", mVContronls.Sigma, 1.0f, 20.0f, 0.1f);
    widget.var("BSigma", mVContronls.BSigma, 0.02f, 1.0f, 0.01f); 
    widget.var("MSize", mVContronls.MSize, 3u, 31u, 2u);
}

void STSM_BilateralFilter::createPassResouces()
{
    mVFilterPass.mpPass = FullScreenPass::create(kPassfile);
    mVFilterPass.mpFbo = Fbo::create();
}
