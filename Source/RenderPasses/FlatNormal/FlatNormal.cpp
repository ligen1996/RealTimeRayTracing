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
#include "FlatNormal.h"


namespace
{
    const char kDesc[] = "Flat Normal";

    const ChannelList kOutChannels =
    {
        { "FlatNormalW", "gNormalW",  "World normal", false, ResourceFormat::RGBA32Float},
    };

    const std::string kPassfile = "RenderPasses/FlatNormal/FlatNormal.slang";
}

// Don't remove this. it's required for hot-reload to function properly
extern "C" __declspec(dllexport) const char* getProjDir()
{
    return PROJECT_DIR;
}

extern "C" __declspec(dllexport) void getPasses(Falcor::RenderPassLibrary& lib)
{
    lib.registerClass("FlatNormal", kDesc, FlatNormal::create);
}

FlatNormal::FlatNormal()
{
    GraphicsProgram::Desc Desc;
    Desc.addShaderLibrary(kPassfile);
    Desc.vsEntry("vsMain").psEntry("psMain");

    mPass.pProgram = GraphicsProgram::create(Desc);
    mPass.pState = GraphicsState::create();
    mPass.pState->setProgram(mPass.pProgram);
}

FlatNormal::SharedPtr FlatNormal::create(RenderContext* pRenderContext, const Dictionary& dict)
{
    SharedPtr pPass = SharedPtr(new FlatNormal);
    return pPass;
}

std::string FlatNormal::getDesc() { return kDesc; }

Dictionary FlatNormal::getScriptingDictionary()
{
    return Dictionary();
}

RenderPassReflection FlatNormal::reflect(const CompileData& compileData)
{
    // Define the required resources here
    RenderPassReflection reflector;
    addRenderPassOutputs(reflector, kOutChannels, ResourceBindFlags::ShaderResource | ResourceBindFlags::RenderTarget);
    return reflector;
}

void FlatNormal::execute(RenderContext* pRenderContext, const RenderData& renderData)
{
    if (!mpScene) return;

    if (!mPass.pFbo)
    {
        std::vector<Texture::SharedPtr> TexColorSet;
        for (int i = 0; i < kOutChannels.size(); ++i)
        {
            std::string TexKey = kOutChannels[i].name;
            Texture::SharedPtr pTex = renderData[TexKey]->asTexture();
            TexColorSet.emplace_back(pTex);
        }

        mPass.pFbo = Fbo::create(TexColorSet);
        mPass.pState->setFbo(mPass.pFbo);
    }

    // renderData holds the requested resources
    // auto& pTexture = renderData["src"]->asTexture();
    for (int i = 0; i < kOutChannels.size(); ++i)
    {
        std::string TexKey = kOutChannels[i].name;
        Texture::SharedPtr pTex = renderData[TexKey]->asTexture();
        mPass.pFbo->attachColorTarget(pTex, i);
    }
    pRenderContext->clearFbo(mPass.pFbo.get(), float4(0.0), 1, 0, FboAttachmentType::Color);
    pRenderContext->clearFbo(mPass.pFbo.get(), float4(1.0), 1, 0, FboAttachmentType::Depth);
    mpScene->rasterize(pRenderContext, mPass.pState.get(), mPass.pVars.get());
}

void FlatNormal::renderUI(Gui::Widgets& widget)
{
}

void FlatNormal::setScene(RenderContext* pRenderContext, const Scene::SharedPtr& pScene)
{
    mpScene = pScene;
    mPass.pProgram->addDefines(pScene->getSceneDefines());
    mPass.pVars = GraphicsVars::create(mPass.pProgram->getReflector());
}
