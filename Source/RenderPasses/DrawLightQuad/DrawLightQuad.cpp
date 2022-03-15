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
#include "DrawLightQuad.h"


namespace
{
    const char kDesc[] = "Pass to draw quad that present the light.";

    const std::string kProgramFile = "RenderPasses/DrawLightQuad/DrawLightQuad.slang";
    const std::string shaderModel = "6_2";

    const std::string kOut = "LQuad";
}

// Don't remove this. it's required for hot-reload to function properly
extern "C" __declspec(dllexport) const char* getProjDir()
{
    return PROJECT_DIR;
}

extern "C" __declspec(dllexport) void getPasses(Falcor::RenderPassLibrary& lib)
{
    lib.registerClass("DrawLightQuad", kDesc, DrawLightQuad::create);
}

DrawLightQuad::DrawLightQuad()
{
    // set pass resource
    Program::Desc desc;
    desc.addShaderLibrary(kProgramFile).vsEntry("vsMain").psEntry("psMain");
    desc.setShaderModel(shaderModel);
    mpProgram = GraphicsProgram::create(desc);

    mpGraphicsState = GraphicsState::create();
    mpGraphicsState->setProgram(mpProgram);

    mpFbo = Fbo::create();

    // init scene
    SceneBuilder::SharedPtr pSceneBuilder = SceneBuilder::create();
    Material::SharedPtr pMat = Material::create("white");
    mpQuad = TriangleMesh::createQuad();
    SceneBuilder::Node MeshNode;
    MeshNode.name = "LightQuad";
    Transform TF;
    TF.setRotationEuler(float3(10,20,30));
    TF.setTranslation(float3(0, 0, 0));
    MeshNode.transform = TF.getMatrix();

    auto MeshID = pSceneBuilder->addTriangleMesh(mpQuad,pMat);
    auto MeshNodeID = pSceneBuilder->addNode(MeshNode);
    pSceneBuilder->addMeshInstance(MeshNodeID,MeshID);

    Camera::SharedPtr pCam = Camera::create("Main Camera");
    pSceneBuilder->addCamera(pCam);

    __setScene(pSceneBuilder->getScene());

    // after all is done, should take care with shader related
    mpProgram->addDefines(mpScene->getSceneDefines());
    mpVars = GraphicsVars::create(mpProgram.get());
}

DrawLightQuad::SharedPtr DrawLightQuad::create(RenderContext* pRenderContext, const Dictionary& dict)
{
    SharedPtr pPass = SharedPtr(new DrawLightQuad);
    return pPass;
}

std::string DrawLightQuad::getDesc() { return kDesc; }

Dictionary DrawLightQuad::getScriptingDictionary()
{
    return Dictionary();
}

RenderPassReflection DrawLightQuad::reflect(const CompileData& compileData)
{
    // Define the required resources here
    RenderPassReflection reflector;
    reflector.addOutput(kOut, "Light Quad");
    //reflector.addInput("src");
    return reflector;
}

void DrawLightQuad::execute(RenderContext* pRenderContext, const RenderData& renderData)
{
    // renderData holds the requested resources
    // auto& pTexture = renderData["src"]->asTexture();
    auto pOutTex = renderData[kOut]->asTexture();
    mpFbo->attachColorTarget(pOutTex,0);
    pRenderContext->clearFbo(mpFbo.get(), float4(0.3,0.8,0.2,0), 1.f, 0, FboAttachmentType::All);

    if (mpLight == nullptr) return;

    mpGraphicsState->setFbo(mpFbo);

    mpScene->rasterize(pRenderContext, mpGraphicsState.get(), mpVars.get(),RasterizerState::CullMode::None);
}

void DrawLightQuad::renderUI(Gui::Widgets& widget)
{
}

void DrawLightQuad::setScene(RenderContext* pRenderContext, const Scene::SharedPtr& pScene)
{
    assert(pScene);
    //mpScene = pScene;
    //mpProgram->addDefines(mpScene->getSceneDefines());
    //mpVars = GraphicsVars::create(mpProgram.get());

    if (!(mpLight = std::dynamic_pointer_cast<RectLight>(pScene->getLightByName("RectLight"))))
    {
        mpLight = std::dynamic_pointer_cast<RectLight>(pScene && pScene->getLightCount() ? pScene->getLight(0) : nullptr);
    }
    assert(mpLight);
}

void DrawLightQuad::__setScene(const Scene::SharedPtr& pScene)
{
    mpScene = pScene;

    if (mpScene)
    {
        const auto& pFbo = gpFramework->getTargetFbo();
        float ratio = float(pFbo->getWidth()) / float(pFbo->getHeight());
        mpScene->setCameraAspectRatio(ratio);

        // create common texture sampler
        Sampler::Desc desc;
        desc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Linear);
        desc.setMaxAnisotropy(8);
        auto pSampler = Sampler::create(desc);
        
        mpScene->bindSamplerToMaterials(pSampler);
    }
}
