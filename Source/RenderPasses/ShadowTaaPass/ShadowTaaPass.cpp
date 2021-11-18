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
#include "ShadowTaaPass.h"
#include "RenderGraph/RenderPassStandardFlags.h"
#include "RenderGraph/RenderPassHelpers.h"
#include <limits>

namespace
{
    const char kDesc[] = "Perform spatial and temporal filtering for Visibility";

    const std::string kColorIn = "colorIn";
    const std::string kVisibilityIn = "visibilityIn";
    const std::string kShadowMotionVector = "ShadowMotionVector";
    //const std::string kColorOut = "colorOut"; //result with shadow
    const std::string kVisiblityOut = "visibilityOut";
    const std::string kDebug = "Debug";

    const ChannelList kPassOutChannels =
    {
        {kVisiblityOut,  "gVisibilily", "filtered Visibility", true, ResourceFormat::RGBA16Float},
        {kDebug,           "gTest",    "Debug Use",            true,   ResourceFormat::RGBA16Float},
    };


    const std::string kAlpha = "alpha";
    const std::string kBoxSigma = "BoxSigma";

    const std::string kShaderFileName = "RenderPasses/ShadowTaaPass/ShadowTaa.ps.slang";

}

// Don't remove this. it's required for hot-reload to function properly
extern "C" __declspec(dllexport) const char* getProjDir()
{
    return PROJECT_DIR;
}

extern "C" __declspec(dllexport) void getPasses(Falcor::RenderPassLibrary& lib)
{
    lib.registerClass("ShadowTaaPass", kDesc, ShadowTaaPass::create);
}

ShadowTaaPass::SharedPtr ShadowTaaPass::create(RenderContext* pRenderContext, const Dictionary& dict)
{
    SharedPtr pPass = SharedPtr(new ShadowTaaPass);
    for (const auto& [key, value] : dict)
    {
        if (key == kAlpha) pPass->mControls.alpha = value;
        else if (key == kBoxSigma) pPass->mControls.BoxSigma = value;
        else logWarning("Unknown field '" + key + "' in a ShadowTaa dictionary");
    }

    return pPass;
}

std::string ShadowTaaPass::getDesc() { return kDesc; }

Dictionary ShadowTaaPass::getScriptingDictionary()
{
    Dictionary dict;
    dict[kAlpha] = mControls.alpha;
    dict[kBoxSigma] = mControls.BoxSigma;

    return dict;
}

RenderPassReflection ShadowTaaPass::reflect(const CompileData& compileData)
{
    // Define the required resources here
    RenderPassReflection reflector;
    reflector.addInput(kShadowMotionVector, "Screen-space visibility motion vector");
    reflector.addInput(kVisibilityIn, "Visibility-buffer of the current frame");
    //reflector.addOutput(kVisiblityOut, "Temporal-filtered visibility buffer");
    addRenderPassOutputs(reflector, kPassOutChannels, Resource::BindFlags::RenderTarget);

    //todo:add some internal 

    return reflector;
}

void ShadowTaaPass::execute(RenderContext* pRenderContext, const RenderData& renderData)
{
    // renderData holds the requested resources
    // auto& pTexture = renderData["src"]->asTexture();

    const auto& pVBufferIn = renderData[kVisibilityIn]->asTexture();
    const auto& pVBufferOut = renderData[kVisiblityOut]->asTexture();
    const auto& pVMotionVec = renderData[kShadowMotionVector]->asTexture();
    const auto& pVDebug = renderData[kDebug]->asTexture();


    allocatePrevVbuffer(pVBufferOut.get());
    mpShadowTaaFbo->attachColorTarget(pVBufferOut, 0);
    mpShadowTaaFbo->attachColorTarget(pVDebug, 1);

    //pRenderContext->clearFbo(mpShadowTaaFbo.get(), float4(0), 1.0f, 0, FboAttachmentType::Color);

    //make sure dimensions match
    assert((pVBufferIn->getWidth() == mpPrevVBuffer->getWidth()) && (pVBufferIn->getWidth() == pVMotionVec->getWidth()));
    assert((pVBufferIn->getHeight() == mpPrevVBuffer->getHeight()) && (pVBufferIn->getHeight() == pVMotionVec->getHeight()));
    assert(pVBufferIn->getSampleCount() == 1 && mpPrevVBuffer->getSampleCount() == 1 && pVMotionVec->getSampleCount() == 1);

    mpShadowTaaPass["PerFrameCB"]["gAlpha"] = mControls.alpha;
    mpShadowTaaPass["PerFrameCB"]["gBoxSigma"] = mControls.BoxSigma;
    mpShadowTaaPass["gTexCurVisibility"] = pVBufferIn;
    mpShadowTaaPass["gTexPreVisibility"] = mpPrevVBuffer;
    mpShadowTaaPass["gTexVMotionVec"] = pVMotionVec;
    mpShadowTaaPass["gSampler"] = mpLinearSampler;
 
    mpShadowTaaPass->execute(pRenderContext, mpShadowTaaFbo);
    pRenderContext->blit(pVBufferOut->getSRV(), mpPrevVBuffer->getRTV());  
}

void ShadowTaaPass::renderUI(Gui::Widgets& widget)
{
    widget.var("Alpha", mControls.alpha, 0.f, 1.0f, 0.001f);//min-max step
    widget.var("Box Sigma", mControls.BoxSigma, 0.f, 15.f, 0.001f);
}

ShadowTaaPass::ShadowTaaPass()
{
    mpShadowTaaPass = FullScreenPass::create(kShaderFileName);
    mpShadowTaaFbo = Fbo::create();
    Sampler::Desc samplerDesc;
    samplerDesc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Linear);
    mpLinearSampler = Sampler::create(samplerDesc);
}

void ShadowTaaPass::allocatePrevVbuffer(const Texture* pVisibilityOut)
{
    bool allocate = mpPrevVBuffer == nullptr;
    allocate = allocate || (mpPrevVBuffer->getWidth() != pVisibilityOut->getWidth());
    allocate = allocate || (mpPrevVBuffer->getHeight() != pVisibilityOut->getHeight());
    allocate = allocate || (mpPrevVBuffer->getDepth() != pVisibilityOut->getDepth());
    allocate = allocate || (mpPrevVBuffer->getFormat() != pVisibilityOut->getFormat());

    assert(pVisibilityOut->getSampleCount() == 1);

    if (allocate) mpPrevVBuffer = Texture::create2D(pVisibilityOut->getWidth(), pVisibilityOut->getHeight(), pVisibilityOut->getFormat(),
                                                     1, 1, nullptr, Resource::BindFlags::RenderTarget | Resource::BindFlags::ShaderResource);

}
