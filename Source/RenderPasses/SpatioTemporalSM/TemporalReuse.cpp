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
    const std::string kOutputVisibility = "Visibility";
    const std::string kDebug = "Debug";
   
    // shader file path
    const std::string kTemporalReusePassfile = "RenderPasses/SpatioTemporalSM/VTemporalReuse.ps.slang";
}

STSM_TemporalReuse::STSM_TemporalReuse()
{
    createVReusePassResouces();
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
    reflector.addOutput(kOutputVisibility, "Visibility").bindFlags(Resource::BindFlags::RenderTarget | Resource::BindFlags::ShaderResource).format(ResourceFormat::RGBA32Float).texture2D(0, 0);
    reflector.addOutput(kDebug, "Debug").bindFlags(ResourceBindFlags::RenderTarget).format(ResourceFormat::RGBA32Float).texture2D(0, 0, 0);
    return reflector;
}

void STSM_TemporalReuse::compile(RenderContext* pContext, const CompileData& compileData)
{
}

void STSM_TemporalReuse::execute(RenderContext* vRenderContext, const RenderData& vRenderData)
{
    if (!mpScene) return;

    const auto& pAlpha = __loadReuseFactorTexture(vRenderData); 

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
    mVReusePass.mpPass["PerFrameCB"]["gEnableBlend"] = mVContronls.accumulateBlend;
    mVReusePass.mpPass["PerFrameCB"]["gEnableClamp"] = mVContronls.clamp;
    mVReusePass.mpPass["PerFrameCB"]["gClampSearchRadius"] = mVContronls.clampSearchRadius;
    mVReusePass.mpPass["PerFrameCB"]["gClampExtendRange"] = mVContronls.clampExtendRange;
    mVReusePass.mpPass["PerFrameCB"]["gEnableDiscardByPosition"] = mVContronls.discardByPosition;
    mVReusePass.mpPass["PerFrameCB"]["gEnableDiscardByNormal"] = mVContronls.discardByNormal;
    mVReusePass.mpPass["PerFrameCB"]["gAdaptiveAlpha"] = mVContronls.adaptiveAlpha;
    mVReusePass.mpPass["PerFrameCB"]["gReverseVariation"] = mVContronls.reverseVariation;
    mVReusePass.mpPass["PerFrameCB"]["gAlpha"] = mVContronls.alpha;//blend weight
    mVReusePass.mpPass["PerFrameCB"]["gViewProjMatrix"] = mpScene->getCamera()->getViewProjMatrix();
    mVReusePass.mpPass["gTexVisibility"] = pInputVisibility;
    mVReusePass.mpPass["gTexAlpha"] = pAlpha;
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
    widget.checkbox("Accumulate Blending", mVContronls.accumulateBlend);
    if (mVContronls.accumulateBlend)
    {
        widget.indent(20.0f);
        widget.checkbox("Adaptive Blend Alpha", mVContronls.adaptiveAlpha);
        widget.tooltip("Use variation to adaptively adjust blend alpha.");
        if (mVContronls.adaptiveAlpha)
        {
            widget.checkbox("Reverse Variation", mVContronls.reverseVariation);
            widget.tooltip("Use [1-v] instead of [v] as variation.");
        }
        widget.var((mVContronls.adaptiveAlpha ? "Blend Alpha Range" : "Blend Alpha"), mVContronls.alpha, 0.f, 1.0f, 0.001f);
        widget.indent(-20.0f);
        widget.checkbox("Clamp", mVContronls.clamp);
        if (mVContronls.clamp)
        {
            widget.indent(20.0f);
            widget.var("Clamp Search Radius", mVContronls.clampSearchRadius, 1u, 5u, 1u);
            widget.var("Clamp Extend Range", mVContronls.clampExtendRange, 0.0f, 1.0f, 0.02f);
            widget.indent(-20.0f);
        }
        widget.checkbox("Discard by Position", mVContronls.discardByPosition);
        widget.checkbox("Discard by Normal", mVContronls.discardByNormal);
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
}

void STSM_TemporalReuse::updateBlendWeight()
{
    if (mIterationIndex == 0) return;

    mVContronls.alpha = float(1.0 / mIterationIndex);
    ++mIterationIndex;
}

Texture::SharedPtr STSM_TemporalReuse::__loadReuseFactorTexture(const RenderData& vRenderData)
{
    const InternalDictionary& Dict = vRenderData.getDictionary();
    if (!Dict.keyExists("ReuseFactor")) return nullptr;
    return Dict["ReuseFactor"];
}
