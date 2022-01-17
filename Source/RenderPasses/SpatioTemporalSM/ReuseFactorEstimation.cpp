#include "ReuseFactorEstimation.h"

namespace
{
    const char kDesc[] = "Reuse Factor Estimation pass";

    // input
    const std::string kColor = "Color";
    const std::string kDepth = "Depth";
    const std::string kMotionVector = "MotionVector";

    // internal
    const std::string kPrevColor = "PrevColor";

    // output
    const std::string kAlpha = "Alpha";
   
    // shader file path
    const std::string kPassfile = "RenderPasses/SpatioTemporalSM/ReuseFactorEstimation.ps.slang";
}

STSM_ReuseFactorEstimation::STSM_ReuseFactorEstimation()
{
    mPass.pFbo = Fbo::create();
    mPass.pPass = FullScreenPass::create(kPassfile);
}

STSM_ReuseFactorEstimation::SharedPtr STSM_ReuseFactorEstimation::create(RenderContext* pRenderContext, const Dictionary& dict)
{
    return SharedPtr(new STSM_ReuseFactorEstimation());
}

std::string STSM_ReuseFactorEstimation::getDesc() { return kDesc; }

Dictionary STSM_ReuseFactorEstimation::getScriptingDictionary()
{
    return Dictionary();
}

RenderPassReflection STSM_ReuseFactorEstimation::reflect(const CompileData& compileData)
{
    // Define the required resources here
    RenderPassReflection reflector;
    reflector.addInput(kColor, "Color");
    reflector.addInput(kDepth, "Depth");
    reflector.addInternal(kPrevColor, "PrevColor").bindFlags(ResourceBindFlags::RenderTarget | Resource::BindFlags::ShaderResource);
    reflector.addInputOutput(kMotionVector, "MotionVector");
    reflector.addOutput(kAlpha, "Alpha").bindFlags(ResourceBindFlags::RenderTarget | Resource::BindFlags::ShaderResource).format(ResourceFormat::R32Float).texture2D(0, 0);
    return reflector;
}

void STSM_ReuseFactorEstimation::execute(RenderContext* vRenderContext, const RenderData& vRenderData)
{
    const auto& pColor = vRenderData[kColor]->asTexture();
    const auto& pDepth = vRenderData[kDepth]->asTexture();
    const auto& pMotionVector = vRenderData[kMotionVector]->asTexture();
    const auto& pPrevColor = vRenderData[kPrevColor]->asTexture();
    const auto& pAlpha = vRenderData[kAlpha]->asTexture();

    mPass.pFbo->attachColorTarget(pAlpha, 0);
    vRenderContext->clearFbo(mPass.pFbo.get(), float4(0, 0, 0, 1), 1, 0, FboAttachmentType::All);

    mPass.pPass["gTexColor"] = pColor;
    mPass.pPass["gTexPrevColor"] = pPrevColor;
    mPass.pPass["gTexMotionVector"] = pMotionVector;
   
    mPass.pPass->execute(vRenderContext, mPass.pFbo);

    vRenderContext->blit(pColor->getSRV(), pPrevColor->getRTV());
}

void STSM_ReuseFactorEstimation::renderUI(Gui::Widgets& widget)
{
}
