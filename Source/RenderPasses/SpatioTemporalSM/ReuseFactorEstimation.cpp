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
    const std::string kPrevVariation = "PrevVariation";

    // output
    const std::string kVariation = "Variation";
    const std::string kVarOfVar = "VarOfVar";
   
    // shader file path
    const std::string kEsitimationPassfile = "RenderPasses/SpatioTemporalSM/ReuseFactorEstimation.ps.slang";
    const std::string kFilterPassfile = "RenderPasses/SpatioTemporalSM/CommonFilter.ps.slang";
    const std::string kDiffPassfile = "RenderPasses/SpatioTemporalSM/Diff.ps.slang";
}

STSM_ReuseFactorEstimation::STSM_ReuseFactorEstimation()
{
    mEstimationPass.pFbo = Fbo::create();
    mEstimationPass.pPass = FullScreenPass::create(kEsitimationPassfile);

    mFilterPass.pFbo = Fbo::create();
    mFilterPass.pPass = FullScreenPass::create(kFilterPassfile);

    mVarOfVarPass.pFbo = Fbo::create();
    mVarOfVarPass.pPass = FullScreenPass::create(kDiffPassfile);
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
    reflector.addInternal(kTempPrevVisibility, "TempPrevVisibility").bindFlags(ResourceBindFlags::RenderTarget | Resource::BindFlags::ShaderResource).format(ResourceFormat::R32Float).texture2D(0, 0);
    reflector.addInternal(kPrevVariation, "PrevVariation").bindFlags(ResourceBindFlags::RenderTarget | Resource::BindFlags::ShaderResource).format(ResourceFormat::R32Float).texture2D(0, 0);
    reflector.addOutput(kVariation, "Variation").bindFlags(ResourceBindFlags::RenderTarget | Resource::BindFlags::ShaderResource).format(ResourceFormat::R32Float).texture2D(0, 0);
    reflector.addOutput(kVarOfVar, "VarOfVar").bindFlags(ResourceBindFlags::RenderTarget | Resource::BindFlags::ShaderResource).format(ResourceFormat::R32Float).texture2D(0, 0);
    return reflector;
}

void STSM_ReuseFactorEstimation::execute(RenderContext* vRenderContext, const RenderData& vRenderData)
{
    const auto& pVariation = vRenderData[kVariation]->asTexture();
    const auto& pVarOfVar = vRenderData[kVarOfVar]->asTexture();

    if (mContronls.ForceOutputOne)
    {
        vRenderContext->clearRtv(pVariation->getRTV().get(), float4(1.0, 1.0, 1.0, 1.0));
        vRenderContext->clearRtv(pVarOfVar->getRTV().get(), float4(1.0, 1.0, 1.0, 1.0));
    }
    else
    {
        __executeEstimation(vRenderContext, vRenderData);
        __executeVariationFilters(vRenderContext, vRenderData);
        __executeCalcVarOfVar(vRenderContext, vRenderData);
        __executeFilter(vRenderContext, vRenderData, pVarOfVar, _FILTER_TYPE_MIN, mContronls.VarOfVarMinFilterKernelSize);
        __executeFilter(vRenderContext, vRenderData, pVarOfVar, _FILTER_TYPE_TENT, mContronls.VarOfVarTentFilterKernelSize);
    }

    InternalDictionary& Dict = vRenderData.getDictionary();
    // write to internal data
    if (!mpVariation)
    {
        mpVariation = Texture::create2D(pVariation->getWidth(), pVariation->getHeight(), pVariation->getFormat(), pVariation->getArraySize(), 1, nullptr, ResourceBindFlags::ShaderResource | ResourceBindFlags::RenderTarget);
    }
    vRenderContext->blit(pVariation->getSRV(), mpVariation->getRTV());
    Dict["Variation"] = mpVariation;

    if (!mpVarOfVar)
    {
        mpVarOfVar = Texture::create2D(pVariation->getWidth(), pVariation->getHeight(), pVariation->getFormat(), pVariation->getArraySize(), 1, nullptr, ResourceBindFlags::ShaderResource | ResourceBindFlags::RenderTarget);
    }
    vRenderContext->blit(pVarOfVar->getSRV(), mpVarOfVar->getRTV());
    Dict["VarOfVar"] = mpVarOfVar;
}

void STSM_ReuseFactorEstimation::renderUI(Gui::Widgets& widget)
{
    widget.checkbox("Force Output One", mContronls.ForceOutputOne);
    widget.tooltip("If turned on. The variation will always be 1.0 at all pixels.");
    if (!mContronls.ForceOutputOne)
    {
        widget.var("Max Filter Kernel Size", mContronls.MaxFilterKernelSize, 1u, 9u, 2u);
        widget.var("Tent Filter Kernel Size", mContronls.TentFilterKernelSize, 3u, 19u, 2u);
        widget.var("VarOfVar Min Filter Kernel Size", mContronls.VarOfVarMinFilterKernelSize, 1u, 15u, 2u);
        widget.var("VarOfVar Tent Filter Kernel Size", mContronls.VarOfVarTentFilterKernelSize, 3u, 31u, 2u);
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
    const auto& pVariation = vRenderData[kVariation]->asTexture();

    mEstimationPass.pFbo->attachColorTarget(pVariation, 0);
    vRenderContext->clearFbo(mEstimationPass.pFbo.get(), float4(0, 0, 0, 1), 1, 0, FboAttachmentType::All);

    mEstimationPass.pPass["gTexVisibility"] = pVis;
    mEstimationPass.pPass["gTexPrevVisibility"] = pPrevVis;
    mEstimationPass.pPass["gTexMotionVector"] = pMotionVector;

    mEstimationPass.pPass->execute(vRenderContext, mEstimationPass.pFbo);

    if (!HasPrevInput)
        vRenderContext->blit(pVis->getSRV(), pPrevVis->getRTV());
}

void STSM_ReuseFactorEstimation::__executeVariationFilters(RenderContext* vRenderContext, const RenderData& vRenderData)
{
    const auto& pVariation = vRenderData[kVariation]->asTexture();
    __executeFilter(vRenderContext, vRenderData, pVariation, _FILTER_TYPE_MAX, mContronls.MaxFilterKernelSize);
    __executeFilter(vRenderContext, vRenderData, pVariation, _FILTER_TYPE_TENT, mContronls.TentFilterKernelSize);
}

void STSM_ReuseFactorEstimation::__executeFilter(RenderContext* vRenderContext, const RenderData& vRenderData, Texture::SharedPtr vTarget, uint vFilterType, uint vKernelSize)
{
    _prepareTempVariationTexture(vTarget);

    mFilterPass.pPass["PerFrameCB"]["gFilterType"] = vFilterType;
    mFilterPass.pPass["PerFrameCB"]["gKernelSize"] = vKernelSize;

    mFilterPass.pPass["PerFrameCB"]["gDirection"] = 0u;
    mFilterPass.pFbo->attachColorTarget(mFilterPass.pTempValue, 0);
    mFilterPass.pPass["gTexValue"] = vTarget;
    mFilterPass.pPass->execute(vRenderContext, mFilterPass.pFbo);

    mFilterPass.pPass["PerFrameCB"]["gDirection"] = 1u;
    mFilterPass.pFbo->attachColorTarget(vTarget, 0);
    mFilterPass.pPass["gTexValue"] = mFilterPass.pTempValue;
    mFilterPass.pPass->execute(vRenderContext, mFilterPass.pFbo);
}

void STSM_ReuseFactorEstimation::__executeCalcVarOfVar(RenderContext* vRenderContext, const RenderData& vRenderData)
{
    const auto& pVariation = vRenderData[kVariation]->asTexture();
    const auto& pPrevVariation = vRenderData[kPrevVariation]->asTexture();
    const auto& pVarOfVar = vRenderData[kVarOfVar]->asTexture();

    mVarOfVarPass.pFbo->attachColorTarget(pVarOfVar, 0);
    mVarOfVarPass.pPass["gTex1"] = pVariation;
    mVarOfVarPass.pPass["gTex2"] = pPrevVariation;

    mVarOfVarPass.pPass->execute(vRenderContext, mVarOfVarPass.pFbo);

    vRenderContext->blit(pVariation->getSRV(), pPrevVariation->getRTV());
}

void STSM_ReuseFactorEstimation::_prepareTempVariationTexture(Texture::SharedPtr vRefTex)
{
    if (mFilterPass.pTempValue) return;
    mFilterPass.pTempValue = Texture::create2D(vRefTex->getWidth(), vRefTex->getHeight(), ResourceFormat::R32Float, vRefTex->getArraySize(), 1, nullptr, ResourceBindFlags::ShaderResource | ResourceBindFlags::RenderTarget);
}
