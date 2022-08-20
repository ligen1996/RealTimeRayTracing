#include "RTBilateralFilter.h"
#include "BilateralFilterConstants.slangh"

namespace
{
    const char kDesc[] = "Bilateral Filter pass";

    // input
    const std::string kColor = "Color";
    const std::string kNormal = "Normal";
    const std::string kDepth = "Depth";

    // output
    const std::string kResult = "Result";
    const std::string kDebug = "Debug";
   
    // shader file path
    const std::string kPixelPassfile = "RenderPasses/SpatioTemporalReuse/BilateralFilter.ps.slang";
    const std::string kComputePassfile = "RenderPasses/SpatioTemporalReuse/BilateralFilter.cs.slang";
}

Gui::DropdownList BilateralFilter::mDirectionList =
{
    Gui::DropdownValue{ int(EFilterDirection::X), toString(EFilterDirection::X) },
    Gui::DropdownValue{ int(EFilterDirection::Y), toString(EFilterDirection::Y) },
    Gui::DropdownValue{ int(EFilterDirection::BOTH), toString(EFilterDirection::BOTH) }
};

Gui::DropdownList BilateralFilter::mMethodList =
{
    Gui::DropdownValue{ int(EMethod::PIXEL_SHADER), toString(EMethod::PIXEL_SHADER) },
    Gui::DropdownValue{ int(EMethod::COMPUTE_SHADER), toString(EMethod::COMPUTE_SHADER) },
};

#define defProp(VarName) FilterPass.def_property(#VarName, &BilateralFilter::get##VarName, &BilateralFilter::set##VarName)

void BilateralFilter::registerScriptBindings(pybind11::module& m)
{
    pybind11::class_<BilateralFilter, RenderPass, BilateralFilter::SharedPtr> FilterPass(m, "RT_BilateralFilter");

    defProp(Enable);
    defProp(Iteration);
    defProp(SigmaColor);
    defProp(SigmaNormal);
    defProp(SigmaDepth);
    defProp(KernelSize);
    defProp(Adaptive);
    defProp(AdaptiveRatio);
    defProp(AdaptiveShiftRange);
}

#undef defProp

BilateralFilter::BilateralFilter()
{
    __createPassResouces();
}

BilateralFilter::SharedPtr BilateralFilter::create(RenderContext* pRenderContext, const Dictionary& dict)
{
    return SharedPtr(new BilateralFilter());
}

std::string BilateralFilter::getDesc() { return kDesc; }

Dictionary BilateralFilter::getScriptingDictionary()
{
    return Dictionary();
}

RenderPassReflection BilateralFilter::reflect(const CompileData& compileData)
{
    // Define the required resources here
    RenderPassReflection reflector;
    reflector.addInput(kColor, "Color");
    reflector.addInput(kNormal, "Normal");
    reflector.addInput(kDepth, "Depth");
    reflector.addOutput(kResult, "Result").bindFlags(Resource::BindFlags::RenderTarget | Resource::BindFlags::ShaderResource).format(ResourceFormat::RGBA32Float).texture2D(0, 0);
    reflector.addOutput(kDebug, "Debug").bindFlags(Resource::BindFlags::RenderTarget).format(ResourceFormat::RGBA32Float).texture2D(0, 0);
    return reflector;
}

void BilateralFilter::compile(RenderContext* pContext, const CompileData& compileData)
{
}

void BilateralFilter::execute(RenderContext* vRenderContext, const RenderData& vRenderData)
{
    PROFILE("Filter");
    if (!mControls.Enable)
    {
        const auto& pColor = vRenderData[kColor]->asTexture();
        const auto& pResult = vRenderData[kResult]->asTexture();
        vRenderContext->blit(pColor->getSRV(), pResult->getRTV());
        return;
    }

    Texture::SharedPtr pVarOfVar, pVariation;
    if (mControls.Adaptive)
    {
        __loadInternalTexture(vRenderData, pVariation, pVarOfVar);
        if (!pVariation || !pVarOfVar) return;
    }

    switch (mControls.Method)
    {
    case EMethod::PIXEL_SHADER: __executePixelPass(vRenderContext, vRenderData, pVariation, pVarOfVar); break;
    case EMethod::COMPUTE_SHADER: __executeComputePass(vRenderContext, vRenderData, pVariation, pVarOfVar); break;
    default: should_not_get_here();
    }
}

void BilateralFilter::renderUI(Gui::Widgets& widget)
{
    widget.checkbox("Enable", mControls.Enable);
    if (mControls.Enable)
    {
        if (mControls.Method == EMethod::PIXEL_SHADER)
            widget.var("Iteration Num", mControls.Iteration, 1u, 10u, 1u);

        uint MethodIndex = (uint)mControls.Method;
        widget.dropdown("Method", mMethodList, MethodIndex);
        mControls.Method = (EMethod)MethodIndex;

        uint DirectionIndex = (uint)mControls.Direction;
        widget.dropdown("Direction", mDirectionList, DirectionIndex);
        mControls.Direction = (EFilterDirection)DirectionIndex;

        widget.var("Sigma Color", mControls.SigmaColor, 1.0f, 50.0f, 0.1f);
        widget.var("Sigma Normal", mControls.SigmaNormal, 1.0f, 50.0f, 0.1f);
        widget.var("Sigma Depth", mControls.SigmaDepth, 1.0f, 50.0f, 0.1f);
        widget.var("Kernel Size", mControls.KernelSize, 3u, 101u, 2u);
        widget.separator();
        widget.checkbox("Adaptive by SRGM", mControls.Adaptive);
        if (mControls.Adaptive)
        {
            widget.indent(20.0f);
            widget.var("Adaptive Ratio (0 = full dv, 1 = full ddv)", mControls.AdaptiveRatio, 0.0f, 1.0f, 0.01f);
            widget.var("Adaptive Shift Range of kernel size", mControls.AdaptiveShiftRange, 1u, 51u, 1u);
            widget.indent(-20.0f);
        }
    }
}

std::string BilateralFilter::toString(EFilterDirection vType)
{
    switch (vType)
    {
    case EFilterDirection::X: return "X";
    case EFilterDirection::Y: return "Y";
    case EFilterDirection::BOTH: return "BOTH";
    default: should_not_get_here(); return "";
    }
}

std::string BilateralFilter::toString(EMethod vMethod)
{
    switch (vMethod)
    {
    case EMethod::PIXEL_SHADER: return "Pixel Shader";
    case EMethod::COMPUTE_SHADER: return "Compute Shader";
    default: should_not_get_here(); return "";
    }
}

void BilateralFilter::__createPassResouces()
{
    // pixel pass
    mPixelFilterPass.pPass = FullScreenPass::create(kPixelPassfile);
    mPixelFilterPass.pFbo = Fbo::create();

    // compute pass
    ComputeProgram::SharedPtr pProgram = ComputeProgram::createFromFile(kComputePassfile, "main");
    mComputeFilterPass.pVars = ComputeVars::create(pProgram->getReflector());
    mComputeFilterPass.pState = ComputeState::create();
    mComputeFilterPass.pState->setProgram(pProgram);
}

void BilateralFilter::__preparePixelStageTexture(Texture::SharedPtr vTarget)
{
    if (!mPixelFilterPass.pIterStageTexture)
        mPixelFilterPass.pIterStageTexture = Texture::create2D(vTarget->getWidth(), vTarget->getHeight(), vTarget->getFormat(), vTarget->getArraySize(), 1, nullptr, ResourceBindFlags::ShaderResource | ResourceBindFlags::RenderTarget);


    if (!mPixelFilterPass.pTempStageTexture)
        mPixelFilterPass.pTempStageTexture = Texture::create2D(vTarget->getWidth(), vTarget->getHeight(), vTarget->getFormat(), vTarget->getArraySize(), 1, nullptr, ResourceBindFlags::ShaderResource | ResourceBindFlags::RenderTarget);
}

void BilateralFilter::__prepareComputeStageTexture(Texture::SharedPtr vTarget)
{
    _ASSERTE(mControls.Method == EMethod::COMPUTE_SHADER);
    if (mControls.Direction == EFilterDirection::BOTH && mComputeFilterPass.pStageTexture[0] && mComputeFilterPass.pStageTexture[1]) return;
    if (mControls.Direction != EFilterDirection::BOTH && mComputeFilterPass.pStageTexture[0]) return;

    for (int i = 0; i < 2; ++i)
    {
        if (mComputeFilterPass.pStageTexture[i]) continue; // empty
        else if (i == 1 && mControls.Direction != EFilterDirection::BOTH) continue; // 2nd texture is not necessary
        mComputeFilterPass.pStageTexture[i] = Texture::create2D(vTarget->getWidth(), vTarget->getHeight(), vTarget->getFormat(), vTarget->getArraySize(), 1, nullptr, ResourceBindFlags::ShaderResource | ResourceBindFlags::UnorderedAccess);
    }
}

void BilateralFilter::__executePixelPass(RenderContext* vRenderContext, const RenderData& vRenderData, Texture::SharedPtr vVariation, Texture::SharedPtr vVarOfVar)
{
    const auto& pColor = vRenderData[kColor]->asTexture();
    const auto& pNormal = vRenderData[kNormal]->asTexture();
    const auto& pDepth = vRenderData[kDepth]->asTexture();
    const auto& pResult = vRenderData[kResult]->asTexture();
    const auto& pDebug = vRenderData[kDebug]->asTexture();

    mPixelFilterPass.pPass["PerFrameCB"]["gSigmaColor"] = mControls.SigmaColor;
    mPixelFilterPass.pPass["PerFrameCB"]["gSigmaNormal"] = mControls.SigmaNormal;
    mPixelFilterPass.pPass["PerFrameCB"]["gSigmaDepth"] = mControls.SigmaDepth;
    mPixelFilterPass.pPass["PerFrameCB"]["gKernelSize"] = mControls.KernelSize;
    mPixelFilterPass.pPass["PerFrameCB"]["gAdaptive"] = mControls.Adaptive;
    mPixelFilterPass.pPass["PerFrameCB"]["gAdaptiveRange"] = mControls.AdaptiveShiftRange;
    mPixelFilterPass.pPass["gTexNormal"] = pNormal;
    mPixelFilterPass.pPass["gTexDepth"] = pDepth;
    mPixelFilterPass.pPass["gTexVariation"] = vVariation;
    mPixelFilterPass.pPass["gTexVarOfVar"] = vVarOfVar;

    mPixelFilterPass.pFbo->attachColorTarget(pDebug, 1);

    __preparePixelStageTexture(pColor);
    Texture::SharedPtr pSource = pColor;
    Texture::SharedPtr pTarget = pResult;
    for (uint i = 0u; i < mControls.Iteration; ++i)
    {
        __executePixelFilter(vRenderContext, pSource, pTarget, mControls.Direction);
        if (i == 0u)
        {
            pSource = pResult;
            pTarget = mPixelFilterPass.pIterStageTexture;
        }
        else
        {
            auto pTemp = pSource;
            pSource = pTarget;
            pTarget = pTemp;
        }
    }
    if (pSource != pResult)
        vRenderContext->blit(pSource->getSRV(), pResult->getRTV());
}


void BilateralFilter::__executePixelFilter(RenderContext* vRenderContext, Texture::SharedPtr vSource, Texture::SharedPtr vTarget, EFilterDirection vDirection)
{
    if (vDirection != EFilterDirection::BOTH)
    {
        mPixelFilterPass.pPass["gTexColor"] = vSource;
        mPixelFilterPass.pFbo->attachColorTarget(vTarget, 0);
        mPixelFilterPass.pPass["PerFrameCB"]["gDirection"] = (int)vDirection;
        mPixelFilterPass.pPass->execute(vRenderContext, mPixelFilterPass.pFbo);
    }
    else
    {
        __executePixelFilter(vRenderContext, vSource, mPixelFilterPass.pTempStageTexture, EFilterDirection::X);
        __executePixelFilter(vRenderContext, mPixelFilterPass.pTempStageTexture, vTarget, EFilterDirection::Y);
    }
}

void BilateralFilter::__executeComputePass(RenderContext* vRenderContext, const RenderData& vRenderData, Texture::SharedPtr vVariation, Texture::SharedPtr vVarOfVar)
{
    const auto& pColor = vRenderData[kColor]->asTexture();
    const auto& pNormal = vRenderData[kNormal]->asTexture();
    const auto& pDepth = vRenderData[kDepth]->asTexture();
    const auto& pResult = vRenderData[kResult]->asTexture();

    // calculate compute shader param
    uint TexWidth = pResult->getWidth();
    uint TexHeight = pResult->getHeight();

    uint32_t NumGroupX = div_round_up((int)TexWidth, _BILATERAL_THREAD_NUM_X);
    uint32_t NumGroupY = div_round_up((int)TexHeight, _BILATERAL_THREAD_NUM_Y);
    uint3 DispatchDim = uint3(NumGroupX, NumGroupY, 1u);

    __prepareComputeStageTexture(pResult);

    mComputeFilterPass.pVars["PerFrameCB"]["gSigmaColor"] = mControls.SigmaColor;
    mComputeFilterPass.pVars["PerFrameCB"]["gSigmaNormal"] = mControls.SigmaNormal;
    mComputeFilterPass.pVars["PerFrameCB"]["gSigmaDepth"] = mControls.SigmaDepth;
    mComputeFilterPass.pVars["PerFrameCB"]["gKernelSize"] = mControls.KernelSize;
    mComputeFilterPass.pVars["PerFrameCB"]["gAdaptive"] = mControls.Adaptive;
    mComputeFilterPass.pVars["PerFrameCB"]["gAdaptiveRatio"] = mControls.AdaptiveRatio;
    mComputeFilterPass.pVars["PerFrameCB"]["gAdaptiveRange"] = mControls.AdaptiveShiftRange;
    mComputeFilterPass.pVars["gTexNormal"] = pNormal;
    mComputeFilterPass.pVars["gTexDepth"] = pDepth;
    mComputeFilterPass.pVars["gTexVariation"] = vVariation;
    mComputeFilterPass.pVars["gTexVarOfVar"] = vVarOfVar;

    mComputeFilterPass.pVars["gTexColor"] = pColor;
    mComputeFilterPass.pVars["gTexOutput"] = mComputeFilterPass.pStageTexture[0];

    int SrcIndex = 0;
    if (mControls.Direction != EFilterDirection::BOTH)
    {
        mComputeFilterPass.pVars["PerFrameCB"]["gDirection"] = (int)mControls.Direction;
        vRenderContext->dispatch(mComputeFilterPass.pState.get(), mComputeFilterPass.pVars.get(), DispatchDim);
    }
    else
    {
        SrcIndex = 1;
        mComputeFilterPass.pVars["PerFrameCB"]["gDirection"] = (int)EFilterDirection::X;
        vRenderContext->dispatch(mComputeFilterPass.pState.get(), mComputeFilterPass.pVars.get(), DispatchDim);
        mComputeFilterPass.pVars["PerFrameCB"]["gDirection"] = (int)EFilterDirection::Y;
        mComputeFilterPass.pVars["gTexColor"] = mComputeFilterPass.pStageTexture[0];
        mComputeFilterPass.pVars["gTexOutput"] = mComputeFilterPass.pStageTexture[1];
        vRenderContext->dispatch(mComputeFilterPass.pState.get(), mComputeFilterPass.pVars.get(), DispatchDim);
    }
    vRenderContext->blit(mComputeFilterPass.pStageTexture[SrcIndex]->getSRV(), pResult->getRTV());
}

void BilateralFilter::__loadInternalTexture(const RenderData& vRenderData, Texture::SharedPtr& voVariation, Texture::SharedPtr& voVarOfVar)
{
    const InternalDictionary& Dict = vRenderData.getDictionary();
    if (Dict.keyExists("Variation"))
        voVariation = Dict["Variation"];

    if (Dict.keyExists("VarOfVar"))
        voVarOfVar = Dict["VarOfVar"];
}
