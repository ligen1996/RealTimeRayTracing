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
#include "TemporalFilter.h"

namespace
{
    const char kDesc[] = "RayTracing Temporal Reuse pass";

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
    const std::string kTemporalReusePassfile = "RenderPasses/SpatioTemporalReuse/TemporalFilter.ps.slang";
};

Gui::DropdownList TemporalFilter::mReuseSampleTypeList =
{
    Gui::DropdownValue{ int(EReuseSampleType::POINT), "Point" },
    Gui::DropdownValue{ int(EReuseSampleType::BILINEAR), "Bilinear" },
    Gui::DropdownValue{ int(EReuseSampleType::CATMULL), "Catmull-Rom" }
};

#define defProp(VarName) ReusePass.def_property(#VarName, &TemporalFilter::get##VarName, &TemporalFilter::set##VarName)

void TemporalFilter::registerScriptBindings(pybind11::module& m)
{
    pybind11::class_<TemporalFilter, RenderPass, TemporalFilter::SharedPtr> ReusePass(m, "TemporalFilter");

    defProp(AccumulateBlend);
    defProp(Clamp);
    defProp(ClampSearchRadius);
    defProp(ClampExtendRange);
    defProp(DiscardByPositionStrength);
    defProp(DiscardByNormalStrength);
    defProp(AdaptiveAlpha);
    defProp(Alpha);
    defProp(Beta);
    defProp(Ratiodv);
    defProp(Ratioddv);
    defProp(AccumulateBlend);
}

#undef defProp

TemporalFilter::TemporalFilter()
{
    createVReusePassResouces();

    // load params
    //std::string ParamFile = "../../Data/Graph/Params/TubeGrid_dynamic_TemporalReuse.json";
    //std::string ParamFile = "../../Data/Graph/Params/Ghosting-Obj-TR-No-Lagging.json";
    //std::string ParamFile = "../../Data/Graph/Params/Ghosting-Obj-TR.json";
    //std::string ParamFile = "../../../Data/Graph/Params/Best-TR.json";
    //pybind11::dict Dict;
    //if (Helper::parsePassParamsFile(ParamFile, Dict))
    //    __loadParams(Dict);
    //else
    //    msgBox("Load param file [" + ParamFile + "] failed", MsgBoxType::Ok, MsgBoxIcon::Error);
}

TemporalFilter::SharedPtr TemporalFilter::create(RenderContext* pRenderContext, const Dictionary& dict)
{
    return SharedPtr(new TemporalFilter());
}

std::string TemporalFilter::getDesc() { return kDesc; }

Dictionary TemporalFilter::getScriptingDictionary()
{
    return Dictionary();
}

RenderPassReflection TemporalFilter::reflect(const CompileData& compileData)
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

void TemporalFilter::compile(RenderContext* pContext, const CompileData& compileData)
{
}

void TemporalFilter::execute(RenderContext* vRenderContext, const RenderData& vRenderData)
{
    PROFILE("TemporalReuse");
    if (!mpScene) return;

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
    mVReusePass.mpPass["PerFrameCB"]["gDiscardByPositionStrength"] = mVControls.discardByPositionStrength;
    mVReusePass.mpPass["PerFrameCB"]["gDiscardByNormalStrength"] = mVControls.discardByNormalStrength;
    mVReusePass.mpPass["PerFrameCB"]["gAdaptiveAlpha"] = mVControls.adaptiveAlpha;
    mVReusePass.mpPass["PerFrameCB"]["gBeta"] = mVControls.beta;
    mVReusePass.mpPass["PerFrameCB"]["gAlpha"] = mVControls.alpha;//blend weight
    mVReusePass.mpPass["PerFrameCB"]["gViewProjMatrix"] = mpScene->getCamera()->getViewProjMatrix();
    mVReusePass.mpPass["PerFrameCB"]["gRatiodv"] = mVControls.ratiodv;
    mVReusePass.mpPass["PerFrameCB"]["gRatioddv"] = mVControls.ratioddv;
    mVReusePass.mpPass["gTexVisibility"] = pInputVisibility;
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

void TemporalFilter::renderUI(Gui::Widgets& widget)
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
        widget.var("Discard by Position Strength", mVControls.discardByPositionStrength, 0.0f, 5.0f, 0.01f);
        widget.var("Discard by Normal Strength", mVControls.discardByNormalStrength, 0.0f, 5.0f, 0.01f);
    }
}

void TemporalFilter::setScene(RenderContext* pRenderContext, const Scene::SharedPtr& pScene)
{
    mpScene = pScene;
    _ASSERTE(mpScene);
}

void TemporalFilter::createVReusePassResouces()
{
    mVReusePass.mpPass = FullScreenPass::create(kTemporalReusePassfile);
    mVReusePass.mpFbo = Fbo::create();

    Sampler::Desc Desc;
    Desc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Linear);
    mpSamplerLinear = Sampler::create(Desc);
    mVReusePass.mpPass["gSamplerLinear"] = mpSamplerLinear;
}

void TemporalFilter::updateBlendWeight()
{
    if (mIterationIndex == 0) return;

    mVControls.alpha = float(1.0 / mIterationIndex);
    ++mIterationIndex;
}


void TemporalFilter::__loadParams(const pybind11::dict& Dict)
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

pybind11::dict TemporalFilter::__getParams()
{
    pybind11::dict Dict;
    Dict["PassName"] = kDesc;
    Dict["Alpha"] = mVControls.alpha;
    Dict["Max_Alpha"] = mVControls.beta;
    Dict["Ratio_dv"] = mVControls.ratiodv;
    Dict["Ratio_ddv"] = mVControls.ratioddv;
    return Dict;
}