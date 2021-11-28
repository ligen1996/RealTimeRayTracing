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
#include "SpatioTemporalFilter.h"
#include "RenderGraph/RenderPassStandardFlags.h"
#include "RenderGraph/RenderPassHelpers.h"
#include <limits>

namespace
{
    const char kDesc[] = "Spatial Filtering pass";

    const std::string kVisibilityIn = "VisibilityIn";
    const std::string kNormalIn = "Normal";
    const std::string kPositionIn = "Position";

    const std::string kVisibilityOut = "VisibilityOut";

    const ChannelList KPassOutChannels =
    {
        {kVisibilityOut, "gVisibility", "filtered Visibilty", true , ResourceFormat::RGBA16Float},
    };


    //const std::string kShaderFileName = "RenderPasses/SpatioTemporalFilter/SpatialFilter.ps.slang";
    const std::string kShaderFileName = "RenderPasses/SpatioTemporalFilter/SpatialFilterTest.ps.slang";


    const std::string  kKernelRadius = "gKernelRadius";
    const std::string  kSigmaCoord = "gSigmaCoord";
    const std::string  kSigmaColor = "gSigmaColor";
    const std::string  kSigmaPlane = "gSigmaPlane";
    const std::string  kSigmaNormals = "gSigmaNormals";
}

// Don't remove this. it's required for hot-reload to function properly
extern "C" __declspec(dllexport) const char* getProjDir()
{
    return PROJECT_DIR;
}

extern "C" __declspec(dllexport) void getPasses(Falcor::RenderPassLibrary& lib)
{
    lib.registerClass("SpatioTemporalFilter", kDesc, SpatioTemporalFilter::create);
}

SpatioTemporalFilter::SharedPtr SpatioTemporalFilter::create(RenderContext* pRenderContext, const Dictionary& dict)
{
    SharedPtr pPass = SharedPtr(new SpatioTemporalFilter);

    for (const auto& [key, value] : dict)
    {
        if (key == kKernelRadius) pPass->mControls.gKernelRadius = value;
        else if (key == kSigmaCoord) pPass->mControls.gSigmaCoord = value;
        else if (key == kSigmaColor) pPass->mControls.gSigmaColor = value;
        else if (key == kSigmaPlane) pPass->mControls.gSigmaPlane = value;
        else if (key == kSigmaNormals) pPass->mControls.gSigmaNormals = value;
        else logWarning("Unknown field '" + key + "' in a SpatialFilter dictionary");
    }

    return pPass;
}

std::string SpatioTemporalFilter::getDesc() { return kDesc; }

Dictionary SpatioTemporalFilter::getScriptingDictionary()
{
    Dictionary dict;
    dict[kKernelRadius] = mControls.gKernelRadius;
    dict[kSigmaCoord] = mControls.gSigmaCoord;
    dict[kSigmaColor] = mControls.gSigmaColor;
    dict[kSigmaNormals] = mControls.gSigmaNormals;
    dict[kSigmaPlane] = mControls.gSigmaPlane;

    return dict;
}

RenderPassReflection SpatioTemporalFilter::reflect(const CompileData& compileData)
{
    // Define the required resources here
    RenderPassReflection reflector;
    reflector.addInput(kPositionIn, "Gbuffer Position");
    reflector.addInput(kVisibilityIn, "Noisy Visility");
    reflector.addInput(kNormalIn, "Gbuffer Normal");

    addRenderPassOutputs(reflector, KPassOutChannels, Resource::BindFlags::RenderTarget);

    return reflector;
}

void SpatioTemporalFilter::execute(RenderContext* pRenderContext, const RenderData& renderData)
{
    // renderData holds the requested resources
    // auto& pTexture = renderData["src"]->asTexture();

    const auto& pVBufferIn = renderData[kVisibilityIn]->asTexture();
    const auto& pVBufferOut = renderData[kVisibilityOut]->asTexture();
    const auto& pNormalIn = renderData[kNormalIn]->asTexture();
    const auto& pPositionIn = renderData[kPositionIn]->asTexture();

    mpSpatialFilterPassFbo->attachColorTarget(pVBufferOut, 0);


    mpSpatialFilterPass["PerFrameCB"]["gKernelRadius"] = mControls.gKernelRadius;
    mpSpatialFilterPass["PerFrameCB"]["gSigmaCoord"] = mControls.gSigmaCoord;
    mpSpatialFilterPass["PerFrameCB"]["gSigmaColor"] = mControls.gSigmaColor;
    mpSpatialFilterPass["PerFrameCB"]["gSigmaPlane"] = mControls.gSigmaPlane;
    mpSpatialFilterPass["PerFrameCB"]["gSigmaNormals"] = mControls.gSigmaNormals;

    mpSpatialFilterPass["gVTex"] = pVBufferIn;
    mpSpatialFilterPass["gNormalTex"] = pNormalIn;
    mpSpatialFilterPass["gPositionTex"] = pPositionIn;



    mpSpatialFilterPass->execute(pRenderContext, mpSpatialFilterPassFbo);
}

void SpatioTemporalFilter::renderUI(Gui::Widgets& widget)
{
    widget.var("KernelRadius", mControls.gKernelRadius, 0, 100, 1);
}

SpatioTemporalFilter::SpatioTemporalFilter()
{
    mpSpatialFilterPass = FullScreenPass::create(kShaderFileName);
    mpSpatialFilterPassFbo = Fbo::create();

}
