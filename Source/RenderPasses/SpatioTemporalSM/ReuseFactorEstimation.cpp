#include "ReuseFactorEstimation.h"
#include "ReuseFactorEstimationConstant.slangh"

namespace
{
    const char kDesc[] = "Reuse Factor Estimation pass";

    // input
    const std::string kVisibility = "Visibility";
    const std::string kPrevVisibility = "PrevVisibility";
    const std::string kMotionVector = "MotionVector";

    // internal
    const std::string kTempPrevVisibility = "TempPrevVisibility";

    // output
    const std::string kAlpha = "Alpha";
   
    // shader file path
    const std::string kEsitimationPassfile = "RenderPasses/SpatioTemporalSM/ReuseFactorEstimation.ps.slang";
    const std::string kFilterPassfile = "RenderPasses/SpatioTemporalSM/VariationFilter.ps.slang";
}

STSM_ReuseFactorEstimation::STSM_ReuseFactorEstimation()
{
    mEstimationPass.pFbo = Fbo::create();
    mEstimationPass.pPass = FullScreenPass::create(kEsitimationPassfile);

    mFilterPass.pFbo = Fbo::create();
    mFilterPass.pPass = FullScreenPass::create(kFilterPassfile);
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
    reflector.addInput(kVisibility, "Visibility");
    reflector.addInput(kPrevVisibility, "PrevVisibility").flags(RenderPassReflection::Field::Flags::Optional);
    reflector.addInput(kMotionVector, "MotionVector");
    reflector.addInternal(kTempPrevVisibility, "TempPrevVisibility").bindFlags(ResourceBindFlags::RenderTarget | Resource::BindFlags::ShaderResource);
    reflector.addOutput(kAlpha, "Alpha").bindFlags(ResourceBindFlags::RenderTarget | Resource::BindFlags::ShaderResource).format(ResourceFormat::R32Float).texture2D(0, 0);
    return reflector;
}

void STSM_ReuseFactorEstimation::execute(RenderContext* vRenderContext, const RenderData& vRenderData)
{
    const auto& pAlpha = vRenderData[kAlpha]->asTexture();

    if (mContronls.ForceOutputOne)
    {
        vRenderContext->clearRtv(pAlpha->getRTV().get(), float4(1.0, 1.0, 1.0, 1.0));
    }
    else
    {
        __executeEstimation(vRenderContext, vRenderData);
        __executeFilters(vRenderContext, vRenderData);
    }


    // write to internal data
    if (!mpAlpha)
    {
        mpAlpha = Texture::create2D(pAlpha->getWidth(), pAlpha->getHeight(), pAlpha->getFormat(), pAlpha->getArraySize(), 1, nullptr, ResourceBindFlags::ShaderResource | ResourceBindFlags::RenderTarget);
    }
    vRenderContext->blit(pAlpha->getSRV(), mpAlpha->getRTV());
    InternalDictionary& Dict = vRenderData.getDictionary();
    Dict["ReuseFactor"] = pAlpha;
}

void STSM_ReuseFactorEstimation::renderUI(Gui::Widgets& widget)
{
    widget.checkbox("Force Output One", mContronls.ForceOutputOne);
    widget.tooltip("If turned on. The alpha will always be 1.0 at all pixels.");
    if (!mContronls.ForceOutputOne)
    {
        widget.var("Max Filter Kernel Size", mContronls.MaxFilterKernelSize, 1u, 9u, 2u);
        widget.var("Tent Filter Kernel Size", mContronls.TentFilterKernelSize, 3u, 19u, 2u);
    }
}

void STSM_ReuseFactorEstimation::__executeEstimation(RenderContext* vRenderContext, const RenderData& vRenderData)
{
    Texture::SharedPtr pVis = vRenderData[kVisibility]->asTexture();
    Texture::SharedPtr pPrevVis = vRenderData[kPrevVisibility]->asTexture();
    bool HasPrevInput = (pPrevVis != nullptr);

    if (!HasPrevInput) // no prev texture, use vis in previous frame
    {
        pPrevVis = vRenderData[kTempPrevVisibility]->asTexture();
    }

    if (!pVis || !pPrevVis) return;

    const auto& pMotionVector = vRenderData[kMotionVector]->asTexture();
    const auto& pAlpha = vRenderData[kAlpha]->asTexture();

    mEstimationPass.pFbo->attachColorTarget(pAlpha, 0);
    vRenderContext->clearFbo(mEstimationPass.pFbo.get(), float4(0, 0, 0, 1), 1, 0, FboAttachmentType::All);

    mEstimationPass.pPass["gTexVisibility"] = pVis;
    mEstimationPass.pPass["gTexPrevVisibility"] = pPrevVis;
    mEstimationPass.pPass["gTexMotionVector"] = pMotionVector;

    mEstimationPass.pPass->execute(vRenderContext, mEstimationPass.pFbo);

    if (!HasPrevInput)
        vRenderContext->blit(pVis->getSRV(), pPrevVis->getRTV());
}

void STSM_ReuseFactorEstimation::__executeFilters(RenderContext* vRenderContext, const RenderData& vRenderData)
{
    __executeFilter(vRenderContext, vRenderData, _FILTER_TYPE_MAX, mContronls.MaxFilterKernelSize);
    __executeFilter(vRenderContext, vRenderData, _FILTER_TYPE_TENT, mContronls.TentFilterKernelSize);
}


void STSM_ReuseFactorEstimation::__executeFilter(RenderContext* vRenderContext, const RenderData& vRenderData, uint vFilterType, uint vKernelSize)
{
    const auto& pAlpha = vRenderData[kAlpha]->asTexture();
    _prepareTempAlphaTexture(pAlpha);

    mFilterPass.pPass["PerFrameCB"]["gFilterType"] = vFilterType;
    mFilterPass.pPass["PerFrameCB"]["gKernelSize"] = vKernelSize;

    mFilterPass.pPass["PerFrameCB"]["gDirection"] = 0u;
    mFilterPass.pFbo->attachColorTarget(mFilterPass.pTempAlpha, 0);
    mFilterPass.pPass["gTexVariation"] = pAlpha;
    mFilterPass.pPass->execute(vRenderContext, mFilterPass.pFbo);

    mFilterPass.pPass["PerFrameCB"]["gDirection"] = 1u;
    mFilterPass.pFbo->attachColorTarget(pAlpha, 0);
    mFilterPass.pPass["gTexVariation"] = mFilterPass.pTempAlpha;
    mFilterPass.pPass->execute(vRenderContext, mFilterPass.pFbo);
}

void STSM_ReuseFactorEstimation::_prepareTempAlphaTexture(Texture::SharedPtr vAlpha)
{
    if (mFilterPass.pTempAlpha) return;
    mFilterPass.pTempAlpha = Texture::create2D(vAlpha->getWidth(), vAlpha->getHeight(), vAlpha->getFormat(), vAlpha->getArraySize(), 1, nullptr, ResourceBindFlags::ShaderResource | ResourceBindFlags::RenderTarget);
}
