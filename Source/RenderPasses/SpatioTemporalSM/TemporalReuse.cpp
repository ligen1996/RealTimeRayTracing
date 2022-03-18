/***************************************************************************
 # Copyright (c) 2015-21, NVIDIA CORPORATION. All rights reserved.
 #
 # Redistribution and use in source and binary forms, with or without
 # modification, are permitted provided that the following conditions
 # are met:
 #  * Redistributions of source code must retain the above copyright
 #    notice, this list of conditions and the following disclaimer.
 #  * Redistributions in binary form must reproduce the above copyright
 #    notice, this list of conditions and the following disclaimer in the
 #    documentation and/or other materials provided with the distribution.
 #  * Neither the name of NVIDIA CORPORATION nor the names of its
 #    contributors may be used to endorse or promote products derived
 #    from this software without specific prior written permission.
 #
 # THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS "AS IS" AND ANY
 # EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 # IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 # PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 # CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 # EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 # PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 # PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 # OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 # (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 # OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **************************************************************************/
#include "TemporalReuse.h"
#include "PassParams.h"

namespace
{
    const char kDesc[] = "Temporal Reuse Visibility pass";

    // input
    const std::string kInputVisibility = "Visibility";
    const std::string kMotionVector = "Motion Vector";
    const std::string kCurPos = "Position";
    const std::string kCurNormal = "Normal";

    // internal
    const std::string kPrevVisibility = "PrevVisbility";
    const std::string kPrevPos = "PrevPos";
    const std::string kPrevNormal = "PrevNormal";

    // output
    const std::string kOutputVisibility = "TR_Visibility";
    const std::string kDebug = "Debug";

    // shader file path
    const std::string kTemporalReusePassfile = "RenderPasses/SpatioTemporalSM/VTemporalReuse.ps.slang";
};

Gui::DropdownList STSM_TemporalReuse::mReuseSampleTypeList =
{
    Gui::DropdownValue{ int(EReuseSampleType::POINT), "Point" },
    Gui::DropdownValue{ int(EReuseSampleType::BILINEAR), "Bilinear" },
    Gui::DropdownValue{ int(EReuseSampleType::CATMULL), "Catmull-Rom" }
};

#define defProp(VarName) ReusePass.def_property(#VarName, &STSM_TemporalReuse::get##VarName, &STSM_TemporalReuse::set##VarName)

void STSM_TemporalReuse::registerScriptBindings(pybind11::module& m)
{
    pybind11::class_<STSM_TemporalReuse, RenderPass, STSM_TemporalReuse::SharedPtr> ReusePass(m, "STSM_TemporalReuse");

    defProp(AccumulateBlend);
    defProp(Clamp);
    defProp(ClampSearchRadius);
    defProp(ClampExtendRange);
    defProp(DiscardByPosition);
    defProp(DiscardByPositionStrength);
    defProp(DiscardByNormal);
    defProp(DiscardByNormalStrength);
    defProp(AdaptiveAlpha);
    defProp(Alpha);
    defProp(Beta);
    defProp(Ratiodv);
    defProp(Ratioddv);
    defProp(AccumulateBlend);
}

#undef defProp

STSM_TemporalReuse::STSM_TemporalReuse()
{
    createVReusePassResouces();

    // load params
    //std::string ParamFile = "../../Data/Graph/Params/TubeGrid_dynamic_TemporalReuse.json";
    //std::string ParamFile = "../../Data/Graph/Params/Ghosting-Obj-TR-No-Lagging.json";
    std::string ParamFile = "../../Data/Graph/Params/Ghosting-Obj-TR.json";
    pybind11::dict Dict;
    if (Helper::parsePassParamsFile(ParamFile, Dict))
        __loadParams(Dict);
    else
        msgBox("Load param file [" + ParamFile + "] failed", MsgBoxType::Ok, MsgBoxIcon::Error);
}

STSM_TemporalReuse::SharedPtr STSM_TemporalReuse::create(RenderContext* pRenderContext, const Dictionary& dict)
{
    return SharedPtr(new STSM_TemporalReuse());
}

std::string STSM_TemporalReuse::getDesc() { return kDesc; }

Dictionary STSM_TemporalReuse::getScriptingDictionary()
{
    return Dictionary();
}

RenderPassReflection STSM_TemporalReuse::reflect(const CompileData& compileData)
{
    // Define the required resources here
    RenderPassReflection reflector;
    reflector.addInput(kInputVisibility, "Visibility");
    reflector.addInput(kMotionVector, "MotionVector");
    reflector.addInput(kCurPos, "CurPos");
    reflector.addInput(kCurNormal, "CurNormal");
    reflector.addInternal(kPrevVisibility, "PrevVisibility").bindFlags(Resource::BindFlags::RenderTarget | Resource::BindFlags::ShaderResource).format(ResourceFormat::RGBA32Float).texture2D(0, 0);
    reflector.addInternal(kPrevPos, "PrevPos").bindFlags(Resource::BindFlags::RenderTarget | Resource::BindFlags::ShaderResource).format(ResourceFormat::RGBA32Float).texture2D(0, 0);
    reflector.addInternal(kPrevNormal, "PrevNormal").bindFlags(Resource::BindFlags::RenderTarget | Resource::BindFlags::ShaderResource).format(ResourceFormat::RGBA32Float).texture2D(0, 0);
    reflector.addOutput(kOutputVisibility, "TR_Visibility").bindFlags(Resource::BindFlags::RenderTarget | Resource::BindFlags::ShaderResource).format(ResourceFormat::RGBA32Float).texture2D(0, 0);
    reflector.addOutput(kDebug, "Debug").bindFlags(ResourceBindFlags::RenderTarget).format(ResourceFormat::RGBA32Float).texture2D(0, 0, 0);
    return reflector;
}

void STSM_TemporalReuse::compile(RenderContext* pContext, const CompileData& compileData)
{
}

void STSM_TemporalReuse::execute(RenderContext* vRenderContext, const RenderData& vRenderData)
{
    if (!mpScene) return;

    Texture::SharedPtr pPrevVariation, pPrevVarOfVar;
    __loadVariationTextures(vRenderData, pPrevVariation, pPrevVarOfVar);

    const auto& pInputVisibility = vRenderData[kInputVisibility]->asTexture();
    const auto& pMotionVector = vRenderData[kMotionVector]->asTexture();
    const auto& pPrevVisibility = vRenderData[kPrevVisibility]->asTexture();
    const auto& pCurPos = vRenderData[kCurPos]->asTexture();
    const auto& pPrevPos = vRenderData[kPrevPos]->asTexture();
    const auto& pCurNormal = vRenderData[kCurNormal]->asTexture();
    const auto& pPrevNormal = vRenderData[kPrevNormal]->asTexture();
    const auto& pOutputVisibility = vRenderData[kOutputVisibility]->asTexture();
    const auto& pDebug = vRenderData[kDebug]->asTexture();

    //Temporal filter Visibility buffer pass
    //updateBlendWeight();
    mVReusePass.mpFbo->attachColorTarget(pOutputVisibility, 0);
    mVReusePass.mpFbo->attachColorTarget(pDebug, 1);
    mVReusePass.mpPass["PerFrameCB"]["gReuseSampleType"] = (uint)mVReusePass.mReuseType;
    mVReusePass.mpPass["PerFrameCB"]["gEnableBlend"] = mVControls.accumulateBlend;
    mVReusePass.mpPass["PerFrameCB"]["gEnableClamp"] = mVControls.clamp;
    mVReusePass.mpPass["PerFrameCB"]["gClampSearchRadius"] = mVControls.clampSearchRadius;
    mVReusePass.mpPass["PerFrameCB"]["gClampExtendRange"] = mVControls.clampExtendRange;
    mVReusePass.mpPass["PerFrameCB"]["gEnableDiscardByPosition"] = mVControls.discardByPosition;
    mVReusePass.mpPass["PerFrameCB"]["gDiscardByPositionStrength"] = mVControls.discardByPositionStrength;
    mVReusePass.mpPass["PerFrameCB"]["gEnableDiscardByNormal"] = mVControls.discardByNormal;
    mVReusePass.mpPass["PerFrameCB"]["gDiscardByNormalStrength"] = mVControls.discardByNormalStrength;
    mVReusePass.mpPass["PerFrameCB"]["gAdaptiveAlpha"] = mVControls.adaptiveAlpha;
    mVReusePass.mpPass["PerFrameCB"]["gBeta"] = mVControls.beta;
    mVReusePass.mpPass["PerFrameCB"]["gAlpha"] = mVControls.alpha;//blend weight
    mVReusePass.mpPass["PerFrameCB"]["gViewProjMatrix"] = mpScene->getCamera()->getViewProjMatrix();
    mVReusePass.mpPass["PerFrameCB"]["gRatiodv"] = mVControls.ratiodv;
    mVReusePass.mpPass["PerFrameCB"]["gRatioddv"] = mVControls.ratioddv;
    mVReusePass.mpPass["gTexVisibility"] = pInputVisibility;
    mVReusePass.mpPass["gTexPrevVariation"] = pPrevVariation;
    mVReusePass.mpPass["gTexPrevVarOfVar"] = pPrevVarOfVar;
    mVReusePass.mpPass["gTexMotionVector"] = pMotionVector;
    mVReusePass.mpPass["gTexPrevVisiblity"] = pPrevVisibility;
    mVReusePass.mpPass["gTexCurPos"] = pCurPos;
    mVReusePass.mpPass["gTexPrevPos"] = pPrevPos;
    mVReusePass.mpPass["gTexCurNormal"] = pCurNormal;
    mVReusePass.mpPass["gTexPrevNormal"] = pPrevNormal;
    mVReusePass.mpPass->execute(vRenderContext, mVReusePass.mpFbo);

    vRenderContext->blit(pOutputVisibility->getSRV(), pPrevVisibility->getRTV());
    vRenderContext->blit(pCurPos->getSRV(), pPrevPos->getRTV());
    vRenderContext->blit(pCurNormal->getSRV(), pPrevNormal->getRTV());
}

void STSM_TemporalReuse::renderUI(Gui::Widgets& widget)
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

    widget.checkbox("Accumulate Blending", mVControls.accumulateBlend);
    if (mVControls.accumulateBlend)
    {
        uint Index = (uint)mVReusePass.mReuseType;
        widget.dropdown("Method", mReuseSampleTypeList, Index);
        mVReusePass.mReuseType = (EReuseSampleType)Index;

        widget.separator();
        widget.indent(20.0f);
        widget.checkbox("Adaptive Blend Alpha", mVControls.adaptiveAlpha);
        widget.tooltip("Use dv and ddv to adaptively adjust blend alpha.");
        widget.var((mVControls.adaptiveAlpha ? "Blend Alpha Range" : "Blend Alpha"), mVControls.alpha, 0.f, 1.0f, 0.001f);
        widget.var("Max Alpha", mVControls.beta, mVControls.alpha, 1.0f, 0.001f);
        if (mVControls.adaptiveAlpha)
        {
            widget.var("Ratio dv", mVControls.ratiodv, 0.0f, 30.0f, 0.01f);
            widget.var("Ratio ddv", mVControls.ratioddv, 0.0f, 30.0f, 0.01f);
        }
        widget.indent(-20.0f);
        widget.checkbox("Clamp", mVControls.clamp);
        if (mVControls.clamp)
        {
            widget.indent(20.0f);
            widget.var("Clamp Search Radius", mVControls.clampSearchRadius, 1u, 21u, 1u);
            widget.var("Clamp Extend Range", mVControls.clampExtendRange, 0.0f, 1.0f, 0.02f);
            widget.indent(-20.0f);
        }
        widget.checkbox("Discard by Position", mVControls.discardByPosition);
        if (mVControls.discardByPosition)
        {
            widget.indent(20.0f);
            widget.var("Discard by Position Strength", mVControls.discardByPositionStrength, 0.1f, 5.0f, 0.01f);
            widget.indent(-20.0f);
        }
        widget.checkbox("Discard by Normal", mVControls.discardByNormal);
        if (mVControls.discardByNormal)
        {
            widget.indent(20.0f);
            widget.var("Discard by Normal Strength", mVControls.discardByNormalStrength, 0.1f, 5.0f, 0.01f);
            widget.indent(-20.0f);
        }
    }
}

void STSM_TemporalReuse::setScene(RenderContext* pRenderContext, const Scene::SharedPtr& pScene)
{
    mpScene = pScene;
    _ASSERTE(mpScene);
}

void STSM_TemporalReuse::createVReusePassResouces()
{
    mVReusePass.mpPass = FullScreenPass::create(kTemporalReusePassfile);
    mVReusePass.mpFbo = Fbo::create();

    Sampler::Desc Desc;
    Desc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Linear);
    mpSamplerLinear = Sampler::create(Desc);
    mVReusePass.mpPass["gSamplerLinear"] = mpSamplerLinear;
}

void STSM_TemporalReuse::updateBlendWeight()
{
    if (mIterationIndex == 0) return;

    mVControls.alpha = float(1.0 / mIterationIndex);
    ++mIterationIndex;
}

void STSM_TemporalReuse::__loadVariationTextures(const RenderData& vRenderData, Texture::SharedPtr& voVariation, Texture::SharedPtr& voVarOfVar)
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

void STSM_TemporalReuse::__loadParams(const pybind11::dict& Dict)
{
    std::string PassName = Dict["PassName"].cast<std::string>();
    if (PassName != kDesc)
    {
        msgBox("Wrong pass param file", MsgBoxType::Ok, MsgBoxIcon::Error);
        return;
    }
    mVControls.alpha = Dict["Alpha"].cast<float>();
    mVControls.beta = Dict["Max_Alpha"].cast<float>();
    mVControls.ratiodv = Dict["Ratio_dv"].cast<float>();
    mVControls.ratioddv = Dict["Ratio_ddv"].cast<float>();
}

pybind11::dict STSM_TemporalReuse::__getParams()
{
    pybind11::dict Dict;
    Dict["PassName"] = kDesc;
    Dict["Alpha"] = mVControls.alpha;
    Dict["Max_Alpha"] = mVControls.beta;
    Dict["Ratio_dv"] = mVControls.ratiodv;
    Dict["Ratio_ddv"] = mVControls.ratioddv;
    return Dict;
}
