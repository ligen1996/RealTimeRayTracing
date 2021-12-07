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
#include "SpatioTemporalSM.h"


namespace
{
    const char kDesc[] = "Simple Shadow map pass";

    //internel paramters
    const std::string kMapSize = "mapSize";
    const std::string kDepthFormat = "depthFormat";

    //input
    const std::string kDepth = "Depth Buffer";

    //output
    const std::string kShadowMap = "ShadowMap";
    const std::string kVisibility = "Visibility";
    const std::string kDebug = "Debug";


    //shader file path
    const std::string kShadowPassfile = "RenderPasses/SpatioTemporalSM/ShadowPass.ps.slang";
    const std::string kVisibilityPassfile = "RenderPasses/SpatioTemporalSM/VisibilityPass.ps.slang";
}


static void createShadowMatrix(const DirectionalLight* pLight, const float3& center, float radius, glm::mat4& shadowVP)
{
    glm::mat4 view = glm::lookAt(center, center + pLight->getWorldDirection(), float3(0, 1, 0));
    glm::mat4 proj = glm::ortho(-radius, radius, -radius, radius, -radius, radius);

    shadowVP = proj * view;
}

static void createShadowMatrix(const PointLight* pLight, const float3& center, float radius, float fboAspectRatio, glm::mat4& shadowVP)
{
    const float3 lightPos = pLight->getWorldPosition();
    const float3 lookat = pLight->getWorldDirection() + lightPos;
    float3 up(0, 1, 0);
    if (abs(glm::dot(up, pLight->getWorldDirection())) >= 0.95f)
    {
        up = float3(1, 0, 0);
    }

    glm::mat4 view = glm::lookAt(lightPos, lookat, up);
    float distFromCenter = glm::length(lightPos - center);
    float nearZ = std::max(0.1f, distFromCenter - radius);
    float maxZ = std::min(radius * 2, distFromCenter + radius);
    float angle = pLight->getOpeningAngle() * 2;
    glm::mat4 proj = glm::perspective(angle, fboAspectRatio, nearZ, maxZ);

    shadowVP = proj * view;
}

static void createShadowMatrix(const Light* pLight, const float3& center, float radius, float fboAspectRatio, glm::mat4& shadowVP)
{
    switch (pLight->getType())
    {
    case LightType::Directional:
        return createShadowMatrix((DirectionalLight*)pLight, center, radius, shadowVP);
    case LightType::Point:
        return createShadowMatrix((PointLight*)pLight, center, radius, fboAspectRatio, shadowVP);
    default:
        should_not_get_here();
    }
}
// Don't remove this. it's required for hot-reload to function properly
extern "C" __declspec(dllexport) const char* getProjDir()
{
    return PROJECT_DIR;
}

extern "C" __declspec(dllexport) void getPasses(Falcor::RenderPassLibrary& lib)
{
    lib.registerClass("SpatioTemporalSM", kDesc, SpatioTemporalSM::create);
}

SpatioTemporalSM::SharedPtr SpatioTemporalSM::create(RenderContext* pRenderContext, const Dictionary& dict)
{
    auto pCSM = SharedPtr(new SpatioTemporalSM());
    return pCSM;
}

std::string SpatioTemporalSM::getDesc() { return kDesc; }

Dictionary SpatioTemporalSM::getScriptingDictionary()
{
    return Dictionary();
}

RenderPassReflection SpatioTemporalSM::reflect(const CompileData& compileData)
{
    // Define the required resources here
    RenderPassReflection reflector;
    reflector.addInput(kDepth, "depth");
    reflector.addInternal(kShadowMap, "shadowMap").bindFlags(Resource::BindFlags::DepthStencil | Resource::BindFlags::ShaderResource).format(mShadowPass.mDepthFormat).texture2D(mMapSize.x, mMapSize.y,0);
    reflector.addOutput(kVisibility, "Visibility").bindFlags(Resource::BindFlags::RenderTarget).format(ResourceFormat::RGBA32Float).texture2D(0, 0);
    reflector.addOutput(kDebug, "Debug").bindFlags(ResourceBindFlags::RenderTarget).format(ResourceFormat::RGBA32Float).texture2D(0, 0, 0);
    return reflector;
}


void SpatioTemporalSM::compile(RenderContext* pContext, const CompileData& compileData)
{
    mVisibilityPass.pFbo->attachColorTarget(nullptr, 0);
}

void camClipSpaceToWorldSpace(const Camera* pCamera, float3 viewFrustum[8], float3& center, float& radius)
{
    float3 clipSpace[8] =
    {
        float3(-1.0f, 1.0f, 0),
        float3(1.0f, 1.0f, 0),
        float3(1.0f, -1.0f, 0),
        float3(-1.0f, -1.0f, 0),
        float3(-1.0f, 1.0f, 1.0f),
        float3(1.0f, 1.0f, 1.0f),
        float3(1.0f, -1.0f, 1.0f),
        float3(-1.0f, -1.0f, 1.0f),
    };

    glm::mat4 invViewProj = pCamera->getInvViewProjMatrix();
    center = float3(0, 0, 0);

    for (uint32_t i = 0; i < 8; i++)
    {
        float4 crd = invViewProj * float4(clipSpace[i], 1);
        viewFrustum[i] = float3(crd) / crd.w;
        center += viewFrustum[i];
    }

    center *= (1.0f / 8.0f);

    // Calculate bounding sphere radius
    radius = 0;
    for (uint32_t i = 0; i < 8; i++)
    {
        float d = glm::length(center - viewFrustum[i]);
        radius = std::max(d, radius);
    }
}


static bool checkOffset(size_t cbOffset, size_t cppOffset, const char* field)
{
    if (cbOffset != cppOffset)
    {
        logError("CsmData::" + std::string(field) + " CB offset mismatch. CB offset is " + std::to_string(cbOffset) + ", C++ data offset is " + std::to_string(cppOffset));
        return false;
    }
    return true;
}

#if _LOG_ENABLED
#define check_offset(_a) {static bool b = true; if(b) {assert(checkOffset(pCB["gSTsmData"][#_a].getByteOffset(), offsetof(StsmData, _a), #_a));} b = false;}
#else
#define check_offset(_a)
#endif


void SpatioTemporalSM::execute(RenderContext* pRenderContext, const RenderData& renderData)
{
    // renderData holds the requested resources
    // auto& pTexture = renderData["src"]->asTexture();
    if (!mpScene || !mpLight) return;

    setupVisibilityPassFbo(renderData[kVisibility]->asTexture());

    const auto& pShadowMap = renderData[kShadowMap]->asTexture();
    const auto& pDepth = renderData[kDepth]->asTexture();
    const auto& pDebug = renderData[kDebug]->asTexture();
    const auto pCamera = mpScene->getCamera().get();
    const auto& pVisBuffer = renderData[kVisibility]->asTexture();

    executeShadowPass(pRenderContext, pShadowMap);//todo:

    //Visibility pass
    //clear visibility buffer
    pRenderContext->clearFbo(mVisibilityPass.pFbo.get(), float4(1, 0, 0, 0), 1, 0, FboAttachmentType::All);

    mVisibilityPass.pFbo->attachColorTarget(pDebug,1);

    //update vars
    mVisibilityPass.pPass["gDepth"] = pDepth; //todo this is form depth 

    auto visibilityVars = mVisibilityPass.pPass->getVars().getRootVar();
    setDataIntoVars(visibilityVars, visibilityVars["PerFrameCB"]["gSTsmData"]);
    mVisibilityPassData.camInvViewProj = pCamera->getInvViewProjMatrix();
    mVisibilityPassData.screenDim = uint2(mVisibilityPass.pFbo->getWidth(), mVisibilityPass.pFbo->getHeight());
    mVisibilityPass.pPass["PerFrameCB"][mVisibilityPass.mPassDataOffset].setBlob(mVisibilityPassData);
    mVisibilityPass.pPass["gShadowMap"] = pShadowMap;

    // Render visibility buffer
    mVisibilityPass.pPass->execute(pRenderContext, mVisibilityPass.pFbo);

}

void SpatioTemporalSM::renderUI(Gui::Widgets& widget)
{
    widget.var("Depth Bias", mSMData.depthBias, 0.000f, 0.1f, 0.0005f);
}

void SpatioTemporalSM::setScene(RenderContext* pRenderContext, const Scene::SharedPtr& pScene)
{
    mpScene = pScene;
    setLight(mpScene && mpScene->getLightCount() ? mpScene->getLight(0) : nullptr); //todo random sample

    if (mpScene) mShadowPass.mpState->getProgram()->addDefines(mpScene->getSceneDefines());

    mShadowPass.mpVars = GraphicsVars::create(mShadowPass.mpState->getProgram()->getReflector());
    const auto& pReflector = mShadowPass.mpVars->getReflection();
    const auto& pDefaultBlock = pReflector->getDefaultParameterBlock();
    mPerLightCbLoc = pDefaultBlock->getResourceBinding("PerLightCB");
}

SpatioTemporalSM::SpatioTemporalSM()
{
    createShadowPassResource();
    createVisibilityPassResource();

    Sampler::Desc samplerDesc;
    samplerDesc.setFilterMode(Sampler::Filter::Point, Sampler::Filter::Point, Sampler::Filter::Point).setAddressingMode(Sampler::AddressMode::Border, Sampler::AddressMode::Border, Sampler::AddressMode::Border).setBorderColor(float4(1.0f));
    samplerDesc.setLodParams(0.f, 0.f, 0.f);
    samplerDesc.setComparisonMode(Sampler::ComparisonMode::LessEqual);
    mShadowPass.pPointCmpSampler = Sampler::create(samplerDesc);
    samplerDesc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Linear);
    mShadowPass.pLinearCmpSampler = Sampler::create(samplerDesc);
    samplerDesc.setComparisonMode(Sampler::ComparisonMode::Disabled);
    samplerDesc.setAddressingMode(Sampler::AddressMode::Clamp, Sampler::AddressMode::Clamp, Sampler::AddressMode::Clamp);
    samplerDesc.setLodParams(-100.f, 100.f, 0.f);
}
void SpatioTemporalSM::createShadowPassResource()
{
    Program::Desc desc;
    desc.addShaderLibrary(kShadowPassfile).vsEntry("vsMain").psEntry("psMain");

    Program::DefineList defines;
    defines.add("TEST_ALPHA");
    defines.add("_APPLY_PROJECTION");
    defines.add("_ALPHA_CHANNEL", "a");

    GraphicsProgram::SharedPtr pProgram = GraphicsProgram::create(desc, defines);
    mShadowPass.mpState = GraphicsState::create();
    mShadowPass.mpState->setProgram(pProgram);
  
    Fbo::Desc fboDesc;
    fboDesc.setDepthStencilTarget(ResourceFormat::D32Float);
    //mShadowPass.mpFbo = Fbo::create();
    mShadowPass.mpFbo = Fbo::create2D(mMapSize.x, mMapSize.y, fboDesc); //todo change shadow map size
    mShadowPass.mpState->setDepthStencilState(nullptr);
    mShadowPass.mpState->setFbo(mShadowPass.mpFbo);


    RasterizerState::Desc rsDesc;
    rsDesc.setDepthClamp(true);
    RasterizerState::SharedPtr rsState = RasterizerState::create(rsDesc);
    mShadowPass.mpState->setRasterizerState(rsState);


    mShadowPass.fboAspectRatio = (float)mMapSize.x / (float)mMapSize.y;
    mShadowPass.mapSize = mMapSize;

}

void SpatioTemporalSM::createVisibilityPassResource()
{
    mVisibilityPass.pFbo = Fbo::create();
    mVisibilityPass.pPass = FullScreenPass::create(kVisibilityPassfile);
    mVisibilityPass.mPassDataOffset = mVisibilityPass.pPass->getVars()->getParameterBlock("PerFrameCB")->getVariableOffset("gPass");
}

void SpatioTemporalSM::setDataIntoVars(ShaderVar const& globalVars, ShaderVar const& csmDataVar)
{
    //csmDataVar["shadowMap"] = mShadowPass.mpFbo->getDepthStencilTexture();
    globalVars["gSTsmCompareSampler"] = mShadowPass.pLinearCmpSampler;
    Sampler::Desc SamplerDesc;
    SamplerDesc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Linear);
    globalVars["gTextureSampler"] = Sampler::create(SamplerDesc);
    csmDataVar.setBlob(mSMData);
}

void SpatioTemporalSM::executeShadowPass(RenderContext* pRenderContext, Texture::SharedPtr pTexture)
{
  
    //const auto pCamera = mpScene->getCamera().get();
    //glm::mat4 ViewProjMat = pCamera->getViewProjMatrix();

    //if (isFirstFrame)
    //{
    //    mSMData.globalMat = ViewProjMat;
    //    isFirstFrame = false;
    //}

    //calcLightViewInfo(pCamera); //calc light mat as csm do

    //get Light Camera,if not exist ,return default camera
    auto getLightCamera = [this]() {
        const auto& Cameras = mpScene->getCameras();
        for (const auto& Camera : Cameras) if (Camera->getName() == "LightCamera") return Camera;
        return Cameras[0];
    };
    mpLightCamera = getLightCamera();
    mSMData.globalMat = mpLightCamera->getViewProjMatrix();
   
    mShadowPass.mpFbo->attachDepthStencilTarget(pTexture);
    mShadowPass.mpState->setFbo(mShadowPass.mpFbo);
    pRenderContext->clearDsv(pTexture->getDSV().get(), 1, 0);

    //calcLightViewInfo(pCamera);
    //mShadowPass.mpVars["PerFrameCB"]["lightProjView"] = mlightProjView;
    mShadowPass.mpVars["PerFrameCB"]["gTest"] = 1.0f;

    GraphicsState::Viewport VP;
    VP.originX = 0;
    VP.originY = 0;
    VP.minDepth = 0;
    VP.maxDepth = 1;
    VP.height = mShadowPass.mapSize.x;
    VP.width = mShadowPass.mapSize.y;

    //Set shadow pass state
    mShadowPass.mpState->setViewport(0, VP);

    auto pCB = mShadowPass.mpVars->getParameterBlock(mPerLightCbLoc);
    check_offset(globalMat);
    pCB->setBlob(&mSMData, 0, sizeof(mSMData));

    if (mpScene) mpScene->rasterize(pRenderContext, mShadowPass.mpState.get(), mShadowPass.mpVars.get());
}

void SpatioTemporalSM::setupVisibilityPassFbo(const Texture::SharedPtr& pVisBuffer)
{
    Texture::SharedPtr pTex = mVisibilityPass.pFbo->getColorTexture(0);
    bool rebind = false;

    if (pVisBuffer && (pVisBuffer != pTex))
    {
        rebind = true;
        pTex = pVisBuffer;
    }
    else if (pTex == nullptr)
    {
        rebind = true;
        ResourceFormat format = ResourceFormat::RGBA32Float;
        pTex = Texture::create2D(mVisibilityPassData.screenDim.x, mVisibilityPassData.screenDim.y, format, 1, 1, nullptr, Resource::BindFlags::RenderTarget | Resource::BindFlags::ShaderResource);
    }

    if (rebind) mVisibilityPass.pFbo->attachColorTarget(pTex, 0);
}

void SpatioTemporalSM::calcLightViewInfo(const Camera* pCamera)
{
    struct
    {
        float3 crd[8];
        float3 center;
        float radius;
    } camFrustum;

    camClipSpaceToWorldSpace(pCamera, camFrustum.crd, camFrustum.center, camFrustum.radius);

    // Create the global shadow space
    createShadowMatrix(mpLight.get(), camFrustum.center, camFrustum.radius, mShadowPass.fboAspectRatio, mSMData.globalMat);
}

void SpatioTemporalSM::setLight(const Light::SharedConstPtr& pLight)
{
    mpLight = pLight;
}







