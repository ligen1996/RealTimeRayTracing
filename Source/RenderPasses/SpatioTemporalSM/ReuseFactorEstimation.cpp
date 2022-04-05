#include "ReuseFactorEstimation.h"
#include "ReuseFactorEstimationConstant.slangh"
#include "PassParams.h"

namespace
{
    const char kDesc[] = "Reuse Factor Estimation pass";

    // input
    const std::string kVisibility = "Visibility_RF";
    const std::string kPrevVisibility = "PrevVisibility_RF";
    const std::string kMotionVector = "MotionVector_RF";
    const std::string kReliability = "Reliability_RF";
    const std::string kPos = "Position_RF";
    const std::string kNormal = "Normal_RF";

    // internal
    const std::string kTempPrevVisibility = "TempPrevVisibility";
    const std::string kPrevVariation = "PrevVariation";
    const std::string kTempVariation = "TempVariation";
    const std::string kPrevPos = "PrevPosition";
    const std::string kPrevNormal = "PrevNormal";

    // output
    const std::string kVariation = "Variation";
    const std::string kVarOfVar = "VarOfVar";
    const std::string kDebug = "Debug";
   
    // shader file path
    const std::string kEsitimationPassfile = "RenderPasses/SpatioTemporalSM/ReuseFactorEstimation.ps.slang";
    const std::string kFilterPassfile = "RenderPasses/SpatioTemporalSM/CommonFilter.ps.slang";
    const std::string kMapPassfile = "RenderPasses/SpatioTemporalSM/CommonMap.ps.slang";
    const std::string kCalcVarOfVarPassfile = "RenderPasses/SpatioTemporalSM/CalcVarOfVar.ps.slang";
    const std::string kFixedAlphaReusePassfile = "RenderPasses/SpatioTemporalSM/CommonFixedAlphaTemporalReuse.ps.slang";
    const std::string kAdaptiveAlphaReusePassfile = "RenderPasses/SpatioTemporalSM/CommonAdaptiveAlphaTemporalReuse.ps.slang";
}

Gui::DropdownList STSM_ReuseFactorEstimation::mReuseSampleTypeList =
{
    Gui::DropdownValue{ int(EReuseSampleType::POINT), "Point" },
    Gui::DropdownValue{ int(EReuseSampleType::BILINEAR), "Bilinear" },
    Gui::DropdownValue{ int(EReuseSampleType::CATMULL), "Catmull-Rom" }
};

#define defProp(VarName) SRGMPass.def_property(#VarName, &STSM_ReuseFactorEstimation::get##VarName, &STSM_ReuseFactorEstimation::set##VarName)

void STSM_ReuseFactorEstimation::registerScriptBindings(pybind11::module& m)
{
    pybind11::class_<STSM_ReuseFactorEstimation, RenderPass, STSM_ReuseFactorEstimation::SharedPtr> SRGMPass(m, "STSM_ReuseFactorEstimation");

    defProp(MaxFilterKernelSize);
    defProp(TentFilterKernelSize);
    defProp(VarOfVarMinFilterKernelSize);
    defProp(VarOfVarMaxFilterKernelSize);
    defProp(VarOfVarTentFilterKernelSize);
    defProp(ReuseVariation);
    defProp(ReuseAlpha);
    defProp(ReuseBeta);
    /*defProp(MapMin);
    defProp(MapMax);*/
    defProp(ReliabilityStrength);
    defProp(Ratiodv);
    defProp(Ratioddv);
    defProp(DiscardByPositionStrength);
    defProp(DiscardByNormalStrength);
    defProp(UseAdaptiveAlpha);
}

#undef defProp

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

    mFixedAlphaReusePass.pFbo = Fbo::create();
    mFixedAlphaReusePass.pPass = FullScreenPass::create(kFixedAlphaReusePassfile);

    mAdaptiveAlphaReusePass.pFbo = Fbo::create();
    mAdaptiveAlphaReusePass.pPass = FullScreenPass::create(kAdaptiveAlphaReusePassfile);

    Sampler::Desc Desc;
    Desc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Linear);
    mpSamplerLinear = Sampler::create(Desc);
    mEstimationPass.pPass["gSamplerLinear"] = mpSamplerLinear;
    mFixedAlphaReusePass.pPass["gSamplerLinear"] = mpSamplerLinear;
    mAdaptiveAlphaReusePass.pPass["gSamplerLinear"] = mpSamplerLinear;

    // load params
    //std::string ParamFile = "../../Data/Graph/Params/TubeGrid_dynamic_SRGM.json";
    //std::string ParamFile = "../../Data/Graph/Params/Ghosting-Obj-RFE.json";
    //std::string ParamFile = "../../Data/Graph/Params/Ghosting-Obj-RFE-No-Lagging.json";
    std::string ParamFile = "../../Data/Graph/Params/Best-RFE.json";
    pybind11::dict Dict;
    if (Helper::parsePassParamsFile(ParamFile, Dict))
        __loadParams(Dict);
    else
        msgBox("Load param file [" + ParamFile + "] failed", MsgBoxType::Ok, MsgBoxIcon::Error);
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
    reflector.addInput(kVisibility, "Visibility_RF");
    reflector.addInput(kPrevVisibility, "PrevVisibility_RF").flags(RenderPassReflection::Field::Flags::Optional);
    reflector.addInput(kMotionVector, "MotionVector_RF");
    reflector.addInput(kReliability, "Reliability_RF");
    reflector.addInput(kPos, "Position_RF");
    reflector.addInput(kNormal, "Normal_RF");
    reflector.addInternal(kTempPrevVisibility, "TempPrevVisibility").bindFlags(ResourceBindFlags::RenderTarget | Resource::BindFlags::ShaderResource).format(ResourceFormat::R32Float).texture2D(0, 0);
    reflector.addInternal(kPrevVariation, "PrevVariation").bindFlags(ResourceBindFlags::RenderTarget | Resource::BindFlags::ShaderResource).format(ResourceFormat::R32Float).texture2D(0, 0);
    reflector.addInternal(kTempVariation, "TempVariation").bindFlags(ResourceBindFlags::RenderTarget | Resource::BindFlags::ShaderResource).format(ResourceFormat::R32Float).texture2D(0, 0);
    reflector.addInternal(kPrevPos, "PrevPos").bindFlags(ResourceBindFlags::RenderTarget | Resource::BindFlags::ShaderResource).format(ResourceFormat::RGBA32Float).texture2D(0, 0);
    reflector.addInternal(kPrevNormal, "PrevNormal").bindFlags(ResourceBindFlags::RenderTarget | Resource::BindFlags::ShaderResource).format(ResourceFormat::RGBA32Float).texture2D(0, 0);
    reflector.addOutput(kVariation, "Variation").bindFlags(ResourceBindFlags::RenderTarget | Resource::BindFlags::ShaderResource).format(ResourceFormat::R32Float).texture2D(0, 0);
    reflector.addOutput(kVarOfVar, "VarOfVar").bindFlags(ResourceBindFlags::RenderTarget | Resource::BindFlags::ShaderResource).format(ResourceFormat::R32Float).texture2D(0, 0);
    reflector.addOutput(kDebug, "Debug").bindFlags(ResourceBindFlags::RenderTarget | Resource::BindFlags::ShaderResource).format(ResourceFormat::RGBA32Float).texture2D(0, 0);
    return reflector;
}

void STSM_ReuseFactorEstimation::execute(RenderContext* vRenderContext, const RenderData& vRenderData)
{
    PROFILE("ReuseFactor");
    if (!mpScene) return;

    const auto& pVariation = vRenderData[kVariation]->asTexture();
    const auto& pPrevVariation = vRenderData[kPrevVariation]->asTexture();
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

        if (mControls.ReuseVariation)
        {
            const auto& pTempVariation = vRenderData[kTempVariation]->asTexture();
            if (mControls.UseAdaptiveAlpha)
                __executeAdaptiveAlphaReuse(vRenderContext, vRenderData, pPrevVariation, pVariation, pTempVariation);
            else
                __executeFixedAlphaReuse(vRenderContext, vRenderData, pPrevVariation, pVariation, pTempVariation, mControls.ReuseAlpha);
            vRenderContext->blit(pTempVariation->getSRV(), pVariation->getRTV());
        }

        __executeCalcVarOfVar(vRenderContext, vRenderData);
        vRenderContext->blit(pVariation->getSRV(), pPrevVariation->getRTV());
    }

    // store position and normal
    const auto& pPos = vRenderData[kPos]->asTexture();
    const auto& pPrevPos = vRenderData[kPrevPos]->asTexture();
    const auto& pNormal = vRenderData[kNormal]->asTexture();
    const auto& pPrevNormal = vRenderData[kPrevNormal]->asTexture();

    vRenderContext->blit(pPos->getSRV(), pPrevPos->getRTV());
    vRenderContext->blit(pNormal->getSRV(), pPrevNormal->getRTV());

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
    if (widget.button("Save Params"))
    {
        Helper::savePassParams(__getParams());
    }

    if (widget.button("Load Params", true))
    {
        pybind11::dict Dict;
        if (Helper::openPassParams(Dict))
        {
            __loadParams(Dict);
        }
    }

    widget.checkbox("Force Output One", mControls.ForceOutputOne);
    widget.tooltip("If turned on. The variation will always be 1.0 at all pixels.");
    if (!mControls.ForceOutputOne)
    {
        widget.var("Var Max Filter Kernel Size", mControls.MaxFilterKernelSize, 1u, 31u, 2u);
        widget.var("Var Tent Filter Kernel Size", mControls.TentFilterKernelSize, 1u, 31u, 2u);
        widget.var("VarOfVar Min Filter Kernel Size", mControls.VarOfVarMinFilterKernelSize, 1u, 51u, 2u);
        widget.var("VarOfVar Max Filter Kernel Size", mControls.VarOfVarMaxFilterKernelSize, 1u, 51u, 2u);
        widget.var("VarOfVar Tent Filter Kernel Size", mControls.VarOfVarTentFilterKernelSize, 1u, 51u, 2u);
        /*widget.var("Map Min", mControls.MapMin, 0.0f, mControls.MapMax, 0.01f);
        widget.var("Map Max", mControls.MapMax, mControls.MapMin, 1.0f, 0.01f);*/
        widget.var("Reliability Strength", mControls.ReliabilityStrength, 0.0f, 1.0f, 0.01f);
        widget.checkbox("Reuse Variation", mControls.ReuseVariation);
        if (mControls.ReuseVariation)
        {
            uint Index = (uint)mControls.ReuseSampleType;
            widget.dropdown("Method", mReuseSampleTypeList, Index);
            mControls.ReuseSampleType = (EReuseSampleType)Index;

            widget.checkbox("Adaptive alpha", mControls.UseAdaptiveAlpha);
            widget.var("Reuse Alpha", mControls.ReuseAlpha, 0.0f, 1.0f, 0.001f);
            if (mControls.UseAdaptiveAlpha)
            {
                widget.var("Max Alpha (Beta)", mControls.ReuseBeta, mControls.ReuseAlpha, 1.0f, 0.001f);
                widget.var("Ratio dv", mControls.Ratiodv, 0.0f, 30.0f, 0.01f);
                widget.var("Ratio ddv", mControls.Ratioddv, 0.0f, 30.0f, 0.01f);
            }
            widget.var("Discard By Position Strength", mControls.DiscardByPositionStrength, 0.0f, 1.0f, 0.01f);
            widget.var("Discard By Normal Strength", mControls.DiscardByNormalStrength, 0.0f, 1.0f, 0.01f);
        }
    }
}

void STSM_ReuseFactorEstimation::setScene(RenderContext* pRenderContext, const Scene::SharedPtr& pScene)
{
    mpScene = pScene;
    _ASSERTE(mpScene);
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
    const auto& pPos = vRenderData[kPos]->asTexture();
    const auto& pPrevPos = vRenderData[kPrevPos]->asTexture();
    const auto& pNormal = vRenderData[kNormal]->asTexture();
    const auto& pPrevNormal = vRenderData[kPrevNormal]->asTexture();

    mEstimationPass.pFbo->attachColorTarget(pVariation, 0);
    vRenderContext->clearFbo(mEstimationPass.pFbo.get(), float4(0, 0, 0, 1), 1, 0, FboAttachmentType::All);

    mEstimationPass.pPass["PerFrameCB"]["gAlpha"] = mControls.ReuseAlpha;
    mEstimationPass.pPass["PerFrameCB"]["gReliabilityStrength"] = mControls.ReliabilityStrength;
    mEstimationPass.pPass["PerFrameCB"]["gDiscardByPositionStrength"] = mControls.DiscardByPositionStrength;
    mEstimationPass.pPass["PerFrameCB"]["gDiscardByNormalStrength"] = mControls.DiscardByNormalStrength;
    mEstimationPass.pPass["PerFrameCB"]["gViewProjMatrix"] = mpScene->getCamera()->getViewProjMatrix();
    mEstimationPass.pPass["PerFrameCB"]["gReuseSampleType"] = (uint)mControls.ReuseSampleType;
    mEstimationPass.pPass["gTexVisibility"] = pVis;
    mEstimationPass.pPass["gTexPrevVisibility"] = pPrevVis;
    mEstimationPass.pPass["gTexMotionVector"] = pMotionVector;
    mEstimationPass.pPass["gTexReliability"] = pReliability;
    mEstimationPass.pPass["gTexCurPos"] = pPos;
    mEstimationPass.pPass["gTexPrevPos"] = pPrevPos;
    mEstimationPass.pPass["gTexCurNormal"] = pNormal;
    mEstimationPass.pPass["gTexPrevNormal"] = pPrevNormal;

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
    _prepareTexture(vTarget, mMapPass.pTempValue);

    mMapPass.pPass["PerFrameCB"]["gMapType"] = vMapType;
    mMapPass.pPass["PerFrameCB"]["gParam1"] = vParam1;
    mMapPass.pPass["PerFrameCB"]["gParam2"] = vParam2;

    mMapPass.pFbo->attachColorTarget(mMapPass.pTempValue, 0);
    mMapPass.pPass["gTexValue"] = vTarget;
    mMapPass.pPass->execute(vRenderContext, mMapPass.pFbo);
    vRenderContext->blit(mMapPass.pTempValue->getSRV(), vTarget->getRTV());
}

void STSM_ReuseFactorEstimation::__executeCalcVarOfVar(RenderContext* vRenderContext, const RenderData& vRenderData)
{
    const auto& pVariation = vRenderData[kVariation]->asTexture();
    const auto& pPrevVariation = vRenderData[kPrevVariation]->asTexture();
    const auto& pVarOfVar = vRenderData[kVarOfVar]->asTexture();
    const auto& pMotionVector = vRenderData[kMotionVector]->asTexture();
    const auto& pReliability = vRenderData[kReliability]->asTexture();

    mVarOfVarPass.pFbo->attachColorTarget(pVarOfVar, 0);
    mVarOfVarPass.pPass["PerFrameCB"]["gReliabilityStrength"] = mControls.ReliabilityStrength;
    mVarOfVarPass.pPass["gTexCur"] = pVariation;
    mVarOfVarPass.pPass["gTexPrev"] = pPrevVariation;
    mVarOfVarPass.pPass["gMotionVector"] = pMotionVector;
    mVarOfVarPass.pPass["gReliability"] = pReliability;

    mVarOfVarPass.pPass->execute(vRenderContext, mVarOfVarPass.pFbo);

    //__executeMap(vRenderContext, vRenderData, pVarOfVar, _MAP_TYPE_CURVE, mControls.MapMin, mControls.MapMax);
    __executeFilter(vRenderContext, vRenderData, pVarOfVar, _FILTER_TYPE_MIN, mControls.VarOfVarMinFilterKernelSize);
    __executeFilter(vRenderContext, vRenderData, pVarOfVar, _FILTER_TYPE_MAX, mControls.VarOfVarMaxFilterKernelSize);
    __executeFilter(vRenderContext, vRenderData, pVarOfVar, _FILTER_TYPE_TENT, mControls.VarOfVarTentFilterKernelSize);
}

void STSM_ReuseFactorEstimation::__executeFixedAlphaReuse(RenderContext* vRenderContext, const RenderData& vRenderData, Texture::SharedPtr vPrev, Texture::SharedPtr vCur, Texture::SharedPtr vTarget, float vAlpha)
{
    const auto& pMotionVector = vRenderData[kMotionVector]->asTexture();
    const auto& pPos = vRenderData[kPos]->asTexture();
    const auto& pPrevPos = vRenderData[kPrevPos]->asTexture();
    const auto& pNormal = vRenderData[kNormal]->asTexture();
    const auto& pPrevNormal = vRenderData[kPrevNormal]->asTexture();
    const auto& pDebug = vRenderData[kDebug]->asTexture();

    mFixedAlphaReusePass.pFbo->attachColorTarget(vTarget, 0);
    mFixedAlphaReusePass.pFbo->attachColorTarget(pDebug, 1);
    mFixedAlphaReusePass.pPass["gTexPrev"] = vPrev;
    mFixedAlphaReusePass.pPass["gTexCur"] = vCur;
    mFixedAlphaReusePass.pPass["gTexMotionVector"] = pMotionVector;
    mFixedAlphaReusePass.pPass["gTexCurPos"] = pPos;
    mFixedAlphaReusePass.pPass["gTexPrevPos"] = pPrevPos;
    mFixedAlphaReusePass.pPass["gTexCurNormal"] = pNormal;
    mFixedAlphaReusePass.pPass["gTexPrevNormal"] = pPrevNormal;
    mFixedAlphaReusePass.pPass["PerFrameCB"]["gAlpha"] = vAlpha;
    mFixedAlphaReusePass.pPass["PerFrameCB"]["gViewProjMatrix"] = mpScene->getCamera()->getViewProjMatrix();
    mFixedAlphaReusePass.pPass["PerFrameCB"]["gDiscardByPositionStrength"] = mControls.DiscardByPositionStrength;
    mFixedAlphaReusePass.pPass["PerFrameCB"]["gDiscardByNormalStrength"] = mControls.DiscardByNormalStrength;
    mFixedAlphaReusePass.pPass["PerFrameCB"]["gReuseSampleType"] = (uint)mControls.ReuseSampleType;
    mFixedAlphaReusePass.pPass->execute(vRenderContext, mFixedAlphaReusePass.pFbo);
}

void STSM_ReuseFactorEstimation::__loadVariationTextures(const RenderData& vRenderData, Texture::SharedPtr& voVariation, Texture::SharedPtr& voVarOfVar)
{
    const InternalDictionary& Dict = vRenderData.getDictionary();
    Texture::SharedPtr pVariation;
    if (Dict.keyExists("Variation"))
        voVariation = Dict["Variation"];
    else
        voVariation = nullptr;

    if (Dict.keyExists("VarOfVar"))
        voVarOfVar = Dict["VarOfVar"];
    else
        voVarOfVar = nullptr;
}

void STSM_ReuseFactorEstimation::__executeAdaptiveAlphaReuse(RenderContext* vRenderContext, const RenderData& vRenderData, Texture::SharedPtr vPrev, Texture::SharedPtr vCur, Texture::SharedPtr vTarget)
{

    Texture::SharedPtr pPrevVariation, pPrevVarOfVar;
    __loadVariationTextures(vRenderData, pPrevVariation, pPrevVarOfVar);

    const auto& pMotionVector = vRenderData[kMotionVector]->asTexture();
    const auto& pPos = vRenderData[kPos]->asTexture();
    const auto& pPrevPos = vRenderData[kPrevPos]->asTexture();
    const auto& pNormal = vRenderData[kNormal]->asTexture();
    const auto& pPrevNormal = vRenderData[kPrevNormal]->asTexture();
    const auto& pDebug = vRenderData[kDebug]->asTexture();

    mAdaptiveAlphaReusePass.pFbo->attachColorTarget(vTarget, 0);
    mAdaptiveAlphaReusePass.pFbo->attachColorTarget(pDebug, 1);
    mAdaptiveAlphaReusePass.pPass["gTexPrevVariation"] = pPrevVariation;
    mAdaptiveAlphaReusePass.pPass["gTexPrevVarOfVar"] = pPrevVarOfVar;
    mAdaptiveAlphaReusePass.pPass["gTexPrev"] = vPrev;
    mAdaptiveAlphaReusePass.pPass["gTexCur"] = vCur;
    mAdaptiveAlphaReusePass.pPass["gTexMotionVector"] = pMotionVector;
    mAdaptiveAlphaReusePass.pPass["gTexCurPos"] = pPos;
    mAdaptiveAlphaReusePass.pPass["gTexPrevPos"] = pPrevPos;
    mAdaptiveAlphaReusePass.pPass["gTexCurNormal"] = pNormal;
    mAdaptiveAlphaReusePass.pPass["gTexPrevNormal"] = pPrevNormal;
    mAdaptiveAlphaReusePass.pPass["PerFrameCB"]["gAlpha"] = mControls.ReuseAlpha;
    mAdaptiveAlphaReusePass.pPass["PerFrameCB"]["gBeta"] = mControls.ReuseBeta;
    mAdaptiveAlphaReusePass.pPass["PerFrameCB"]["gRatiodv"] = mControls.Ratiodv;
    mAdaptiveAlphaReusePass.pPass["PerFrameCB"]["gRatioddv"] = mControls.Ratioddv;
    mAdaptiveAlphaReusePass.pPass["PerFrameCB"]["gViewProjMatrix"] = mpScene->getCamera()->getViewProjMatrix();
    mAdaptiveAlphaReusePass.pPass["PerFrameCB"]["gDiscardByPositionStrength"] = mControls.DiscardByPositionStrength;
    mAdaptiveAlphaReusePass.pPass["PerFrameCB"]["gDiscardByNormalStrength"] = mControls.DiscardByNormalStrength;
    mAdaptiveAlphaReusePass.pPass["PerFrameCB"]["gReuseSampleType"] = (uint)mControls.ReuseSampleType;
    mAdaptiveAlphaReusePass.pPass->execute(vRenderContext, mAdaptiveAlphaReusePass.pFbo);
}

void STSM_ReuseFactorEstimation::_prepareTexture(Texture::SharedPtr vRefTex, Texture::SharedPtr& voTexTarget)
{
    if (voTexTarget) return;
    voTexTarget = Texture::create2D(vRefTex->getWidth(), vRefTex->getHeight(), ResourceFormat::R32Float, vRefTex->getArraySize(), 1, nullptr, ResourceBindFlags::ShaderResource | ResourceBindFlags::RenderTarget);
}


void STSM_ReuseFactorEstimation::__loadParams(const pybind11::dict& Dict)
{
    std::string PassName = Dict["PassName"].cast<std::string>();
    if (PassName != kDesc)
    {
        msgBox("Wrong pass param file", MsgBoxType::Ok, MsgBoxIcon::Error);
        return;
    }
    mControls.MaxFilterKernelSize = Dict["Kernel_dv_Max"].cast<uint>();
    mControls.TentFilterKernelSize = Dict["Kernel_dv_Tent"].cast<uint>();
    mControls.VarOfVarMinFilterKernelSize = Dict["Kernel_ddv_Min"].cast<uint>();
    mControls.VarOfVarMaxFilterKernelSize = Dict["Kernel_ddv_Max"].cast<uint>();
    mControls.VarOfVarTentFilterKernelSize = Dict["Kernel_ddv_Tent"].cast<uint>();
    /*mControls.MapMin = Dict["ddv_map_min"].cast<float>();
    mControls.MapMax = Dict["ddv_map_max"].cast<float>();*/
    mControls.ReliabilityStrength = Dict["Reliability_Strength"].cast<float>();

    mControls.ReuseAlpha = Dict["Alpha"].cast<float>();
    mControls.ReuseBeta = Dict["Max_Alpha"].cast<float>();
    mControls.Ratiodv = Dict["Ratio_dv"].cast<float>();
    mControls.Ratioddv = Dict["Ratio_ddv"].cast<float>();
}

pybind11::dict STSM_ReuseFactorEstimation::__getParams()
{
    pybind11::dict Dict;
    Dict["PassName"] = kDesc;
    Dict["Kernel_dv_Max"] = mControls.MaxFilterKernelSize;
    Dict["Kernel_dv_Tent"] = mControls.TentFilterKernelSize;
    Dict["Kernel_ddv_Min"] = mControls.VarOfVarMinFilterKernelSize;
    Dict["Kernel_ddv_Max"] = mControls.VarOfVarMaxFilterKernelSize;
    Dict["Kernel_ddv_Tent"] = mControls.VarOfVarTentFilterKernelSize;
    /*Dict["ddv_map_min"] = mControls.MapMin;
    Dict["ddv_map_max"] = mControls.MapMax;*/
    Dict["Reliability_Strength"] = mControls.ReliabilityStrength;

    Dict["Alpha"] = mControls.ReuseAlpha;
    Dict["Max_Alpha"] = mControls.ReuseBeta;
    Dict["Ratio_dv"] = mControls.Ratiodv;
    Dict["Ratio_ddv"] = mControls.Ratioddv;
    return Dict;
}
