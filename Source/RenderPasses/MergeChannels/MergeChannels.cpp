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
#include "MergeChannels.h"

namespace
{
    const char kDesc[] = "Merge Four Texture into One Texture";

    const ChannelList kInChannels =
    {
        { "R", "gR",  "Channel R", true /* optional */, ResourceFormat::Unknown},
        { "G", "gG",  "Channel G", true /* optional */, ResourceFormat::Unknown},
        { "B", "gB",  "Channel B", true /* optional */, ResourceFormat::Unknown},
        { "A", "gA",  "Channel A", true /* optional */, ResourceFormat::Unknown},
    };

    const ChannelList kOutChannels =
    {
        { "Out", "gOut",  "Output", true /* optional */, ResourceFormat::RGBA32Float},
    };

    const std::string kShaderFileName = "RenderPasses/MergeChannels/MergeChannels.ps.slang";
}

// Don't remove this. it's required for hot-reload to function properly
extern "C" __declspec(dllexport) const char* getProjDir()
{
    return PROJECT_DIR;
}

extern "C" __declspec(dllexport) void getPasses(Falcor::RenderPassLibrary& lib)
{
    lib.registerClass("MergeChannels", kDesc, MergeChannels::create);
}

MergeChannels::SharedPtr MergeChannels::create(RenderContext* pRenderContext, const Dictionary& dict)
{
    SharedPtr pPass = SharedPtr(new MergeChannels);
    return pPass;
}

std::string MergeChannels::getDesc() { return kDesc; }

Dictionary MergeChannels::getScriptingDictionary()
{
    return Dictionary();
}

RenderPassReflection MergeChannels::reflect(const CompileData& compileData)
{
    // Define the required resources here
    RenderPassReflection reflector;
    addRenderPassInputs(reflector, kInChannels);
    for (const auto& channel : kInChannels)
    {
        auto& buffer = reflector.addInput(channel.name, channel.desc);
        if (channel.optional) buffer.flags(RenderPassReflection::Field::Flags::Optional);
    }

    //addRenderPassOutputs(reflector, kOutChannels);
    for (const auto& channel : kOutChannels)
    {
        auto& buffer = reflector.addOutput(channel.name, channel.desc);
    }
    return reflector;
}

void MergeChannels::execute(RenderContext* pRenderContext, const RenderData& renderData)
{
    // renderData holds the requested resources
    // auto& pTexture = renderData["src"]->asTexture();
    const auto& pOutTex = renderData["Out"]->asTexture();
    mpFbo->attachColorTarget(pOutTex, 0);

    for (const auto& channel : kInChannels)
    {
        Texture::SharedPtr pTex = renderData[channel.name]->asTexture();
        if (pTex == nullptr) // light offset may be empty
        {
            pTex = Texture::create2D(mpFbo->getWidth(), mpFbo->getHeight(), ResourceFormat::RGBA32Float);
            //pRenderContext->clearTexture(pTex.get());
        }
        mpPass[channel.texname] = pTex;
    }


    mpPass->execute(pRenderContext, mpFbo);
}

void MergeChannels::renderUI(Gui::Widgets& widget)
{
}

MergeChannels::MergeChannels()
{
    mpPass = FullScreenPass::create(kShaderFileName);
    mpFbo = Fbo::create();
}
