#include "ReuseFactorEstimation.h"
#include "ReuseFactorEstimationConstant.slangh"

namespace
{
    const char kDesc[] = "Reuse Factor Estimation pass";

    // input
    const std::string kVisibility = "Visibility";
    const std::string kPrevVisibility = "PrevVisibility";
    const std::string kMotionVector = "MotionVector";
    const std::string kReliability = "Reliability";

    // internal
    const std::string kTempPrevVisibility = "TempPrevVisibility";
    const std::string kPrevVariation = "PrevVariation";
    const std::string kTempVarOfVar = "TempVarOfVar";
    const std::string kPrevVarOfVar = "PrevVarOfVar";

    // output
    const std::string kVariation = "Variation";
    const std::string kVarOfVar = "VarOfVar";
   
    // shader file path
    const std::string kEsitimationPassfile = "RenderPasses/SpatioTemporalSM/ReuseFactorEstimation.ps.slang";
    const std::string kFilterPassfile = "RenderPasses/SpatioTemporalSM/CommonFilter.ps.slang";
    const std::string kMapPassfile = "RenderPasses/SpatioTemporalSM/CommonMap.ps.slang";
    const std::string kCalcVarOfVarPassfile = "RenderPasses/SpatioTemporalSM/CalcVarOfVar.ps.slang";
    const std::string kCommonReusePassfile = "RenderPasses/SpatioTemporalSM/CommonTemporalReuse.ps.slang";
}

STSM_ReuseFactorEstimation::STSM_ReuseFactorEstimation()
{
    mEstimationPass.pFbo = Fbo::create();
    mEstimationPass.pPass = FullScreenPass::create(kEsitimationPassfile);

    mFilterPass.pFbo = Fbo::create();
    mFilterPass.pPass = FullScreenPass::create(kFilterPassfile);

    mMapPass.pFbo = Fbo::create();
    mMapPass.pPass = FullScreenPass::create(kMapPassfile);

    mVarOfVarPass.pFbo = Fbo::create();
    mVarOfVarPass.pPass = FullScreenPass::create(kCalcVarOfVarPassfile);

    mCommonReusePass.pFbo = Fbo::create();
    mCommonReusePass.pPass = FullScreenPass::create(kCommonReusePassfile);
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
    reflector.addInput(kReliability, "Reliability");
    reflector.addInternal(kTempPrevVisibility, "TempPrevVisibility").bindFlags(ResourceBindFlags::RenderTarget | Resource::BindFlags::ShaderResource).format(ResourceFormat::R32Float).texture2D(0, 0);
    reflector.addInternal(kPrevVariation, "PrevVariation").bindFlags(ResourceBindFlags::RenderTarget | Resource::BindFlags::ShaderResource).format(ResourceFormat::R32Float).texture2D(0, 0);
    reflector.addInternal(kTempVarOfVar, "TempVarOfVar").bindFlags(ResourceBindFlags::RenderTarget | Resource::BindFlags::ShaderResource).format(ResourceFormat::R32Float).texture2D(0, 0);
    reflector.addInternal(kPrevVarOfVar, "PrevVarOfVar").bindFlags(ResourceBindFlags::RenderTarget | Resource::BindFlags::ShaderResource).format(ResourceFormat::R32Float).texture2D(0, 0);
    reflector.addOutput(kVariation, "Variation").bindFlags(ResourceBindFlags::RenderTarget | Resource::BindFlags::ShaderResource).format(ResourceFormat::R32Float).texture2D(0, 0);
    reflector.addOutput(kVarOfVar, "VarOfVar").bindFlags(ResourceBindFlags::RenderTarget | Resource::BindFlags::ShaderResource).format(ResourceFormat::R32Float).texture2D(0, 0);
    return reflector;
}

void STSM_ReuseFactorEstimation::execute(RenderContext* vRenderContext, const RenderData& vRenderData)
{
    const auto& pVariation = vRenderData[kVariation]->asTexture();
    const auto& pVarOfVar = vRenderData[kVarOfVar]->asTexture();

    if (mControls.ForceOutputOne)
    {
        vRenderContext->clearRtv(pVariation->getRTV().get(), float4(1.0, 1.0, 1.0, 1.0));
        vRenderContext->clearRtv(pVarOfVar->getRTV().get(), float4(1.0, 1.0, 1.0, 1.0));
    }
    else
    {
        __executeEstimation(vRenderContext, vRenderData);
        __executeVariationFilters(vRenderContext, vRenderData);
        __executeCalcVarOfVar(vRenderContext, vRenderData);
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
    widget.checkbox("Force Output One", mControls.ForceOutputOne);
    widget.tooltip("If turned on. The variation will always be 1.0 at all pixels.");
    if (!mControls.ForceOutputOne)
    {
        widget.var("Max Filter Kernel Size", mControls.MaxFilterKernelSize, 1u, 9u, 2u);
        widget.var("Tent Filter Kernel Size", mControls.TentFilterKernelSize, 3u, 19u, 2u);
        widget.var("VarOfVar Min Filter Kernel Size", mControls.VarOfVarMinFilterKernelSize, 1u, 15u, 2u);
        widget.var("VarOfVar Tent Filter Kernel Size", mControls.VarOfVarTentFilterKernelSize, 3u, 31u, 2u);
        widget.var("Map Min", mControls.MapMin, 0.0f, mControls.MapMax, 0.01f);
        widget.var("Map Max", mControls.MapMax, mControls.MapMin, 1.0f, 0.01f);
        widget.checkbox("Reuse Var of Var", mControls.ReuseVarOfVar);
        if (mControls.ReuseVarOfVar)
        {
            widget.var("Reuse Alpha", mControls.ReuseAlpha, 0.0f, 1.0f, 0.001f);
        }
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
    const auto& pReliability = vRenderData[kReliability]->asTexture();

    mEstimationPass.pFbo->attachColorTarget(pVariation, 0);
    vRenderContext->clearFbo(mEstimationPass.pFbo.get(), float4(0, 0, 0, 1), 1, 0, FboAttachmentType::All);

    mEstimationPass.pPass["gTexVisibility"] = pVis;
    mEstimationPass.pPass["gTexPrevVisibility"] = pPrevVis;
    mEstimationPass.pPass["gTexMotionVector"] = pMotionVector;
    mEstimationPass.pPass["gTexReliability"] = pReliability;

    mEstimationPass.pPass->execute(vRenderContext, mEstimationPass.pFbo);

    if (!HasPrevInput)
        vRenderContext->blit(pVis->getSRV(), pPrevVis->getRTV());
}

void STSM_ReuseFactorEstimation::__executeVariationFilters(RenderContext* vRenderContext, const RenderData& vRenderData)
{
    const auto& pVariation = vRenderData[kVariation]->asTexture();
    __executeFilter(vRenderContext, vRenderData, pVariation, _FILTER_TYPE_MAX, mControls.MaxFilterKernelSize);
    __executeFilter(vRenderContext, vRenderData, pVariation, _FILTER_TYPE_TENT, mControls.TentFilterKernelSize);
}

void STSM_ReuseFactorEstimation::__executeFilter(RenderContext* vRenderContext, const RenderData& vRenderData, Texture::SharedPtr vTarget, uint vFilterType, uint vKernelSize)
{
    _prepareTexture(vTarget, mFilterPass.pTempValue);

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

void STSM_ReuseFactorEstimation::__executeMap(RenderContext* vRenderContext, const RenderData& vRenderData, Texture::SharedPtr vTarget, uint vMapType, float vParam1, float vParam2)
{
    const std::string EventName = "Execute min/max map";
    Profiler::instance().startEvent(EventName);
    _prepareTexture(vTarget, mMapPass.pTempValue);

    mMapPass.pPass["PerFrameCB"]["gMapType"] = vMapType;
    mMapPass.pPass["PerFrameCB"]["gParam1"] = vParam1;
    mMapPass.pPass["PerFrameCB"]["gParam2"] = vParam2;

    mMapPass.pFbo->attachColorTarget(mMapPass.pTempValue, 0);
    mMapPass.pPass["gTexValue"] = vTarget;
    mMapPass.pPass->execute(vRenderContext, mMapPass.pFbo);
    vRenderContext->blit(mMapPass.pTempValue->getSRV(), vTarget->getRTV());
    Profiler::instance().endEvent(EventName); 
}

void STSM_ReuseFactorEstimation::__executeCalcVarOfVar(RenderContext* vRenderContext, const RenderData& vRenderData)
{
    const auto& pVariation = vRenderData[kVariation]->asTexture();
    const auto& pPrevVariation = vRenderData[kPrevVariation]->asTexture();
    const auto& pTempVarOfVar = vRenderData[kTempVarOfVar]->asTexture();
    const auto& pPrevVarOfVar = vRenderData[kPrevVarOfVar]->asTexture();
    const auto& pVarOfVar = vRenderData[kVarOfVar]->asTexture();
    const auto& pMotionVector = vRenderData[kMotionVector]->asTexture();
    const auto& pReliability = vRenderData[kReliability]->asTexture();

    Texture::SharedPtr pTarget;
    if (mControls.ReuseVarOfVar)
        pTarget = pTempVarOfVar;
    else
        pTarget = pVarOfVar;

    mVarOfVarPass.pFbo->attachColorTarget(pTarget, 0);
    mVarOfVarPass.pPass["gTexCur"] = pVariation;
    mVarOfVarPass.pPass["gTexPrev"] = pPrevVariation;
    mVarOfVarPass.pPass["gMotionVector"] = pMotionVector;
    mVarOfVarPass.pPass["gReliability"] = pReliability;

    mVarOfVarPass.pPass->execute(vRenderContext, mVarOfVarPass.pFbo);

    __executeMap(vRenderContext, vRenderData, pTarget, _MAP_TYPE_CURVE, mControls.MapMin, mControls.MapMax);
    __executeFilter(vRenderContext, vRenderData, pTarget, _FILTER_TYPE_MIN, mControls.VarOfVarMinFilterKernelSize);
    __executeFilter(vRenderContext, vRenderData, pTarget, _FILTER_TYPE_TENT, mControls.VarOfVarTentFilterKernelSize);

    if (mControls.ReuseVarOfVar)
    {
        mCommonReusePass.pFbo->attachColorTarget(pVarOfVar, 0);
        mCommonReusePass.pPass["gTexPrev"] = pPrevVarOfVar;
        mCommonReusePass.pPass["gTexCur"] = pTempVarOfVar;
        mCommonReusePass.pPass["gTexMotionVector"] = pMotionVector;
        mCommonReusePass.pPass["PerFrameCB"]["gAlpha"] = mControls.ReuseAlpha;
        mCommonReusePass.pPass->execute(vRenderContext, mCommonReusePass.pFbo);

        vRenderContext->blit(pVarOfVar->getSRV(), pPrevVarOfVar->getRTV());
    }

    vRenderContext->blit(pVariation->getSRV(), pPrevVariation->getRTV());
}

void STSM_ReuseFactorEstimation::_prepareTexture(Texture::SharedPtr vRefTex, Texture::SharedPtr& voTexTarget)
{
    if (voTexTarget) return;
    voTexTarget = Texture::create2D(vRefTex->getWidth(), vRefTex->getHeight(), ResourceFormat::R32Float, vRefTex->getArraySize(), 1, nullptr, ResourceBindFlags::ShaderResource | ResourceBindFlags::RenderTarget);
}
