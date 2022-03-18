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
#include "ATrousFilter.h"


namespace
{
    const char kDesc[] = "A-Trous Filter";
    const char kProgramFile[] = "RenderPasses/ATrousFilter/ATrousFilter.ps.slang";

    const ChannelList kInChannels =
    {
        { "Vis"         , "gVisibility" ,  "Visibility"         , false , ResourceFormat::RGBA32Float},
        { "LinearZ"     , "gLinearZ"    ,  "World position"     , false , ResourceFormat::RG32Float},
        { "Normal"      , "gNormal"     ,  "World normal"       , false , ResourceFormat::RGBA32Float},
        //{ "dv"          , "gDV"         ,  "Delta visibility"   , false , ResourceFormat::RGBA32Float},
        //{ "ddv"         , "gDDV"        ,  "Delta of Delta visibility"   , false , ResourceFormat::RGBA32Float}
    };

    const ChannelList kOutChannels =
    {
        { "Filterd"     , "gFiltered"   ,  "Filterd Image"      , false , ResourceFormat::RGBA32Float}
    };
}

// Don't remove this. it's required for hot-reload to function properly
extern "C" __declspec(dllexport) const char* getProjDir()
{
    return PROJECT_DIR;
}

extern "C" __declspec(dllexport) void getPasses(Falcor::RenderPassLibrary& lib)
{
    lib.registerClass("ATrousFilter", kDesc, ATrousFilter::create);
}

ATrousFilter::SharedPtr ATrousFilter::create(RenderContext* pRenderContext, const Dictionary& dict)
{
    SharedPtr pPass = SharedPtr(new ATrousFilter);
    return pPass;
}

std::string ATrousFilter::getDesc() { return kDesc; }

Dictionary ATrousFilter::getScriptingDictionary()
{
    return Dictionary();
}

ATrousFilter::ATrousFilter()
{
    mpPass = FullScreenPass::create(kProgramFile);
    mpFbo = Fbo::create();
}

RenderPassReflection ATrousFilter::reflect(const CompileData& compileData)
{
    // Define the required resources here
    RenderPassReflection reflector;
    addRenderPassInputs(reflector, kInChannels, ResourceBindFlags::ShaderResource);
    addRenderPassOutputs(reflector, kOutChannels, ResourceBindFlags::RenderTarget);
    return reflector;
}

void ATrousFilter::execute(RenderContext* pRenderContext, const RenderData& renderData)
{
    if (!mpScene)
    {
        return;
    }

    const auto& pVisibility = renderData["Vis"]->asTexture();
    const auto& pOutTex = renderData["Filterd"]->asTexture();

    for (const auto& channel : kInChannels)
    {
        if (channel.name == "Vis") continue;
        auto pTex = renderData[channel.name]->asTexture();
        /*if (pTex != nullptr)
        {
            mpPass->addDefine("has_" + channel.texname);
        }
        else
        {
            mpPass->removeDefine("has_" + channel.texname);
        }*/
        mpPass[channel.texname] = pTex;
    }

    mpPass["PerFrameCB"]["g"].setBlob(mPassData);

    Texture::SharedPtr pSource;
    Texture::SharedPtr pTarget;
    __prepareSwapTextures(pVisibility,pSource,pTarget);
    pRenderContext->blit(pVisibility->getSRV(), pSource->getRTV());
    for (uint i = 0u; i < mControls.Iteration; ++i)
    {
        __executeFilter(pRenderContext, pSource, pTarget, 1<<i);
        swap(pSource, pTarget);
    }

    pRenderContext->blit(pSource->getSRV(), pOutTex->getRTV());
}

void ATrousFilter::renderUI(Gui::Widgets& widget)
{
    widget.var("Iteration", mControls.Iteration, 1u,  10u,  1u);
    widget.var("PhiVisibility", mPassData.PhiVisibility, 0.1f, 50.f, 0.1f);
    widget.var("PhiNormal", mPassData.PhiNormal, 0.1f, 200.f, 1.f);
}

void ATrousFilter::__prepareSwapTextures(Texture::SharedPtr vSource, Texture::SharedPtr& voSource, Texture::SharedPtr& voTarget)
{
    voSource = Texture::Texture::create2D(vSource->getWidth(), vSource->getHeight(), vSource->getFormat(), vSource->getArraySize(), 1, nullptr, ResourceBindFlags::ShaderResource | ResourceBindFlags::RenderTarget);

    voTarget = Texture::Texture::create2D(vSource->getWidth(), vSource->getHeight(), vSource->getFormat(), vSource->getArraySize(), 1, nullptr, ResourceBindFlags::ShaderResource | ResourceBindFlags::RenderTarget);
}

void ATrousFilter::__executeFilter(RenderContext* pRenderContext, Texture::SharedPtr& vSource, Texture::SharedPtr& vTarget, int vStepSize)
{
    mpPass["gVisibility"] = vSource;
    mpFbo->attachColorTarget(vTarget, 0);
    mpPass["PerFrameCB"]["gStepSize"] = vStepSize;
    mpPass->execute(pRenderContext, mpFbo);
}
