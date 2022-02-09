#include "BilateralFilter.h"
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
   
    // shader file path
    const std::string kPixelPassfile = "RenderPasses/SpatioTemporalSM/BilateralFilter.ps.slang";
    const std::string kComputePassfile = "RenderPasses/SpatioTemporalSM/BilateralFilter.cs.slang";
}

Gui::DropdownList STSM_BilateralFilter::mDirectionList =
{
    Gui::DropdownValue{ int(EFilterDirection::X), toString(EFilterDirection::X) },
    Gui::DropdownValue{ int(EFilterDirection::Y), toString(EFilterDirection::Y) },
    Gui::DropdownValue{ int(EFilterDirection::BOTH), toString(EFilterDirection::BOTH) }
};

Gui::DropdownList STSM_BilateralFilter::mMethodList =
{
    Gui::DropdownValue{ int(EMethod::PIXEL_SHADER), toString(EMethod::PIXEL_SHADER) },
    Gui::DropdownValue{ int(EMethod::COMPUTE_SHADER), toString(EMethod::COMPUTE_SHADER) },
};

STSM_BilateralFilter::STSM_BilateralFilter()
{
    __createPassResouces();
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

void STSM_BilateralFilter::execute(RenderContext* vRenderContext, const RenderData& vRenderData)
{
    if (!mContronls.Enable)
    {
        const auto& pColor = vRenderData[kColor]->asTexture();
        const auto& pResult = vRenderData[kResult]->asTexture();
        vRenderContext->blit(pColor->getSRV(), pResult->getRTV());
        return;
    }

    Texture::SharedPtr pVarOfVar;
    if (mContronls.AdaptiveByddV)
    {
        pVarOfVar = __loadTextureddV(vRenderData);
        if (!pVarOfVar) return;
    }

    switch (mContronls.Method)
    {
    case EMethod::PIXEL_SHADER: __executePixelPass(vRenderContext, vRenderData, pVarOfVar); break;
    case EMethod::COMPUTE_SHADER: __executeComputePass(vRenderContext, vRenderData, pVarOfVar); break;
    default: should_not_get_here();
    }
}

void STSM_BilateralFilter::renderUI(Gui::Widgets& widget)
{
    widget.checkbox("Enable", mContronls.Enable);
    if (mContronls.Enable)
    {
        uint MethodIndex = (uint)mContronls.Method;
        widget.dropdown("Method", mMethodList, MethodIndex);
        mContronls.Method = (EMethod)MethodIndex;

        uint DirectionIndex = (uint)mContronls.Direction;
        widget.dropdown("Direction", mDirectionList, DirectionIndex);
        mContronls.Direction = (EFilterDirection)DirectionIndex;

        widget.var("Sigma Color", mContronls.SigmaColor, 1.0f, 50.0f, 0.1f);
        widget.var("Sigma Normal", mContronls.SigmaNormal, 1.0f, 50.0f, 0.1f);
        widget.var("Sigma Depth", mContronls.SigmaDepth, 1.0f, 50.0f, 0.1f);
        widget.var("Kernel Size", mContronls.KernelSize, 3u, 101u, 2u);
        widget.separator();
        widget.checkbox("Adaptive by ddV", mContronls.AdaptiveByddV);
        if (mContronls.AdaptiveByddV)
        {
            widget.var("Adaptive Shift Range of kernel size", mContronls.AdaptiveShiftRange, 1u, 51u, 1u);
        }
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

std::string STSM_BilateralFilter::toString(EMethod vMethod)
{
    switch (vMethod)
    {
    case EMethod::PIXEL_SHADER: return "Pixel Shader";
    case EMethod::COMPUTE_SHADER: return "Compute Shader";
    default: should_not_get_here(); return "";
    }
}

void STSM_BilateralFilter::__createPassResouces()
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

void STSM_BilateralFilter::__prepareStageFbo(Texture::SharedPtr vTarget)
{
    if (mPixelFilterPass.pStageFbo) return;
    Fbo::Desc fboDesc;
    fboDesc.setColorTarget(0, vTarget->getFormat());
    mPixelFilterPass.pStageFbo = Fbo::create2D(vTarget->getWidth(), vTarget->getHeight(), fboDesc, vTarget->getArraySize());
}

void STSM_BilateralFilter::__prepareStageTexture(Texture::SharedPtr vTarget)
{
    _ASSERTE(mContronls.Method == EMethod::COMPUTE_SHADER);
    if (mContronls.Direction == EFilterDirection::BOTH && mComputeFilterPass.pStageTexture[0] && mComputeFilterPass.pStageTexture[1]) return;
    if (mContronls.Direction != EFilterDirection::BOTH && mComputeFilterPass.pStageTexture[0]) return;

    for (int i = 0; i < 2; ++i)
    {
        if (mComputeFilterPass.pStageTexture[i]) continue; // empty
        else if (i == 1 && mContronls.Direction != EFilterDirection::BOTH) continue; // 2nd texture is not necessary
        mComputeFilterPass.pStageTexture[i] = Texture::create2D(vTarget->getWidth(), vTarget->getHeight(), vTarget->getFormat(), vTarget->getArraySize(), 1, nullptr, ResourceBindFlags::ShaderResource | ResourceBindFlags::UnorderedAccess);
    }
}

void STSM_BilateralFilter::__executePixelPass(RenderContext* vRenderContext, const RenderData& vRenderData, Texture::SharedPtr vVarOfVar)
{
    const auto& pColor = vRenderData[kColor]->asTexture();
    const auto& pNormal = vRenderData[kNormal]->asTexture();
    const auto& pDepth = vRenderData[kDepth]->asTexture();
    const auto& pResult = vRenderData[kResult]->asTexture();

    mPixelFilterPass.pPass["PerFrameCB"]["gSigmaColor"] = mContronls.SigmaColor;
    mPixelFilterPass.pPass["PerFrameCB"]["gSigmaNormal"] = mContronls.SigmaNormal;
    mPixelFilterPass.pPass["PerFrameCB"]["gSigmaDepth"] = mContronls.SigmaDepth;
    mPixelFilterPass.pPass["PerFrameCB"]["gKernelSize"] = mContronls.KernelSize;
    mPixelFilterPass.pPass["PerFrameCB"]["gAdaptive"] = mContronls.AdaptiveByddV;
    mPixelFilterPass.pPass["PerFrameCB"]["gAdaptiveRange"] = mContronls.AdaptiveShiftRange;
    mPixelFilterPass.pPass["gTexNormal"] = pNormal;
    mPixelFilterPass.pPass["gTexDepth"] = pDepth;
    mPixelFilterPass.pPass["gTexVarOfVar"] = vVarOfVar;

    mPixelFilterPass.pPass["gTexColor"] = pColor;
    mPixelFilterPass.pFbo->attachColorTarget(pResult, 0);
    if (mContronls.Direction != EFilterDirection::BOTH)
    {
        mPixelFilterPass.pPass["PerFrameCB"]["gDirection"] = (int)mContronls.Direction;
        mPixelFilterPass.pPass->execute(vRenderContext, mPixelFilterPass.pFbo);
    }
    else
    {
        __prepareStageFbo(pResult);
        mPixelFilterPass.pPass["PerFrameCB"]["gDirection"] = (int)EFilterDirection::X;
        mPixelFilterPass.pPass->execute(vRenderContext, mPixelFilterPass.pStageFbo);
        mPixelFilterPass.pPass["PerFrameCB"]["gDirection"] = (int)EFilterDirection::Y;
        mPixelFilterPass.pPass["gTexColor"] = mPixelFilterPass.pStageFbo->getColorTexture(0);
        mPixelFilterPass.pPass->execute(vRenderContext, mPixelFilterPass.pFbo);
    }
}

void STSM_BilateralFilter::__executeComputePass(RenderContext* vRenderContext, const RenderData& vRenderData, Texture::SharedPtr vVarOfVar)
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

    __prepareStageTexture(pResult);

    mComputeFilterPass.pVars["PerFrameCB"]["gSigmaColor"] = mContronls.SigmaColor;
    mComputeFilterPass.pVars["PerFrameCB"]["gSigmaNormal"] = mContronls.SigmaNormal;
    mComputeFilterPass.pVars["PerFrameCB"]["gSigmaDepth"] = mContronls.SigmaDepth;
    mComputeFilterPass.pVars["PerFrameCB"]["gKernelSize"] = mContronls.KernelSize;
    mComputeFilterPass.pVars["PerFrameCB"]["gAdaptive"] = mContronls.AdaptiveByddV;
    mComputeFilterPass.pVars["PerFrameCB"]["gAdaptiveRange"] = mContronls.AdaptiveShiftRange;
    mComputeFilterPass.pVars["gTexNormal"] = pNormal;
    mComputeFilterPass.pVars["gTexDepth"] = pDepth;
    mComputeFilterPass.pVars["gTexVarOfVar"] = vVarOfVar;

    mComputeFilterPass.pVars["gTexColor"] = pColor;
    mComputeFilterPass.pVars["gTexOutput"] = mComputeFilterPass.pStageTexture[0];

    int SrcIndex = 0;
    if (mContronls.Direction != EFilterDirection::BOTH)
    {
        mComputeFilterPass.pVars["PerFrameCB"]["gDirection"] = (int)mContronls.Direction;
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

Texture::SharedPtr STSM_BilateralFilter::__loadTextureddV(const RenderData& vRenderData)
{
    const InternalDictionary& Dict = vRenderData.getDictionary();
    Texture::SharedPtr pVariation;
    if (Dict.keyExists("Variation"))
        return Dict["Variation"];
    else
        return nullptr;
}
