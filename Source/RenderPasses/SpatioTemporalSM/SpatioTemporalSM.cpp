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
    const std::string kMotionVector = "Motion Vector Buffer";

    //output
    const std::string kShadowMap = "ShadowMap";
    const std::string kInternalV = "InternelVisibility";
    const std::string kCurPos = "CurPos";
    const std::string kPrevPos = "PrevPos";
    const std::string kCurNormal = "CurNormal";
    const std::string kPrevNormal = "PrevNormal";
    const std::string kDebug = "Debug";

    const std::string kVisibility = "Visibility";
   
    //shader file path
    const std::string kShadowPassfile = "RenderPasses/SpatioTemporalSM/ShadowPass.ps.slang";
    const std::string kVisibilityPassfile = "RenderPasses/SpatioTemporalSM/VisibilityPass.ps.slang";
    const std::string kTemporalReusePassfile = "RenderPasses/SpatioTemporalSM/VTemporalReuse.ps.slang";
}

static CPUSampleGenerator::SharedPtr createSamplePattern(SpatioTemporalSM::SamplePattern type, uint32_t sampleCount)
{
    switch (type)
    {
    case SpatioTemporalSM::SamplePattern::Center:
        return nullptr;
        break;
    case SpatioTemporalSM::SamplePattern::DirectX:
        return DxSamplePattern::create(sampleCount);
        break;
    case SpatioTemporalSM::SamplePattern::Halton:
        return HaltonSamplePattern::create(sampleCount);
        break;
    case SpatioTemporalSM::SamplePattern::Stratitied:
        return StratifiedSamplePattern::create(sampleCount);
        break;
    default:
        should_not_get_here();
        return nullptr;
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
    return Dictionary();//todo
}

RenderPassReflection SpatioTemporalSM::reflect(const CompileData& compileData)
{
    // Define the required resources here
    RenderPassReflection reflector;
    reflector.addInput(kDepth, "depth");
    reflector.addInput(kMotionVector, "MotionVector");
    reflector.addInput(kCurPos, "CurPos");
    reflector.addInput(kCurNormal, "CurNormal");
    reflector.addInternal(kShadowMap, "shadowMap").bindFlags(Resource::BindFlags::DepthStencil | Resource::BindFlags::ShaderResource).format(mShadowPass.mDepthFormat).texture2D(mMapSize.x, mMapSize.y,0);
    reflector.addInternal(kInternalV, "InternelViibility").bindFlags(Resource::BindFlags::RenderTarget | Resource::BindFlags::ShaderResource).format(ResourceFormat::RGBA32Float).texture2D(0, 0);
    reflector.addInternal(kPrevPos, "PrevPos").bindFlags(Resource::BindFlags::RenderTarget | Resource::BindFlags::ShaderResource).format(ResourceFormat::RGBA32Float).texture2D(0, 0);
    reflector.addInternal(kPrevNormal, "PrevNormal").bindFlags(Resource::BindFlags::RenderTarget | Resource::BindFlags::ShaderResource).format(ResourceFormat::RGBA32Float).texture2D(0, 0);
    reflector.addOutput(kVisibility, "Visibility").bindFlags(Resource::BindFlags::RenderTarget | Resource::BindFlags::ShaderResource).format(ResourceFormat::RGBA32Float).texture2D(0, 0);
    reflector.addOutput(kDebug, "Debug").bindFlags(ResourceBindFlags::RenderTarget).format(ResourceFormat::RGBA32Float).texture2D(0, 0, 0);
    return reflector;
}

void SpatioTemporalSM::compile(RenderContext* pContext, const CompileData& compileData)
{
    mVisibilityPass.pFbo->attachColorTarget(nullptr, 0);
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
    if (!mpScene || !mpLight) return;

    const auto pCamera = mpScene->getCamera().get();
    const auto& pShadowMap = renderData[kShadowMap]->asTexture();
    const auto& pDepth = renderData[kDepth]->asTexture();
    const auto& pMotionVector = renderData[kMotionVector]->asTexture();
    const auto& pDebug = renderData[kDebug]->asTexture();
    const auto& pInternalV = renderData[kInternalV]->asTexture();
    const auto& pCurPos = renderData[kCurPos]->asTexture();
    const auto& pPrevPos = renderData[kPrevPos]->asTexture();
    const auto& pCurNormal = renderData[kCurNormal]->asTexture();
    const auto& pPrevNormal = renderData[kPrevNormal]->asTexture();
    const auto& pVisibilityOut = renderData[kVisibility]->asTexture();

    //Shadow pass
    executeShadowPass(pRenderContext, pShadowMap);

    //Visibility pass
    setupVisibilityPassFbo(pInternalV);
    pRenderContext->clearFbo(mVisibilityPass.pFbo.get(), float4(1, 0, 0, 0), 1, 0, FboAttachmentType::All);//clear visibility buffer
    mVisibilityPass.pFbo->attachColorTarget(pDebug,1);
    auto visibilityVars = mVisibilityPass.pPass->getVars().getRootVar();//update vars
    setDataIntoVars(visibilityVars, visibilityVars["PerFrameCB"]["gSTsmData"]);
    mVisibilityPassData.camInvViewProj = pCamera->getInvViewProjMatrix();
    mVisibilityPassData.screenDim = uint2(mVisibilityPass.pFbo->getWidth(), mVisibilityPass.pFbo->getHeight());
    mVisibilityPass.pPass["PerFrameCB"][mVisibilityPass.mPassDataOffset].setBlob(mVisibilityPassData);
    mVisibilityPass.pPass["gShadowMap"] = pShadowMap;
    mVisibilityPass.pPass["gDepth"] = pDepth; 
    mVisibilityPass.pPass["PerFrameCB"]["PcfRadius"] = mPcfRadius;
    mVisibilityPass.pPass->execute(pRenderContext, mVisibilityPass.pFbo); // Render visibility buffer

    //Temporal filter Vibisibility buffer pass
    //updateBlendWeight();
    allocatePrevBuffer(pVisibilityOut.get());
    mVReusePass.mpFbo->attachColorTarget(pVisibilityOut, 0);
    mVReusePass.mpFbo->attachColorTarget(pDebug, 1);
    mVReusePass.mpPass["PerFrameCB"]["gAlpha"] = mVContronls.alpha;//blend weight
    mVReusePass.mpPass["PerFrameCB"]["gViewProjMatrix"] = mpScene->getCamera()->getViewProjMatrix();
    mVReusePass.mpPass["gTexMotionVector"] = pMotionVector;
    mVReusePass.mpPass["gTexVisibility"] = pInternalV;
    mVReusePass.mpPass["gTexPrevVisiblity"] = mVReusePass.mpPrevVisibility;
    mVReusePass.mpPass["gTexCurPos"] = pCurPos;
    mVReusePass.mpPass["gTexPrevPos"] = pPrevPos;
    mVReusePass.mpPass["gTexCurNormal"] = pCurNormal;
    mVReusePass.mpPass["gTexPrevNormal"] = pPrevNormal;
    mVReusePass.mpPass->execute(pRenderContext, mVReusePass.mpFbo);

    pRenderContext->blit(pVisibilityOut->getSRV(), mVReusePass.mpPrevVisibility->getRTV());
    pRenderContext->blit(pCurPos->getSRV(), pPrevPos->getRTV());
    pRenderContext->blit(pCurNormal->getSRV(), pPrevNormal->getRTV());
}

void SpatioTemporalSM::renderUI(Gui::Widgets& widget)
{
    widget.var("Depth Bias", mSMData.depthBias, 0.000f, 0.1f, 0.0005f);
    widget.var("Alpha", mVContronls.alpha, 0.f, 1.0f, 0.001f);
    widget.var("Sample Count", mJitterPattern.mSampleCount, 0u, 1000u, 1u);
    widget.var("PCF Radius", mPcfRadius, 0, 100, 1);
}

void SpatioTemporalSM::setScene(RenderContext* pRenderContext, const Scene::SharedPtr& pScene)
{
    mpScene = pScene;
    _ASSERTE(mpScene);
    auto pLight = mpScene->getLightByName("RectLight");
    _ASSERTE(pLight);
    setLight(pLight);

    /*SceneBuilder::SharedPtr pBuilder = SceneBuilder::create();

    auto cube = Falcor::TriangleMesh::createCube();
    auto quad = Falcor::TriangleMesh::createQuad();

    auto white = Falcor::Material::create("while");
    white->setBaseColor(float4(1, 1, 1,1));
    white->setRoughness(float(0.3));

    auto cubeID = pBuilder->addTriangleMesh(cube,white);
    auto quadID = pBuilder->addTriangleMesh(quad,white);

    SceneBuilder::Node cubeNode;
    Falcor::Transform cubeTrans;
    cubeTrans.setTranslation(float3(0,0,0));
    cubeNode.name = "cube";
    cubeNode.transform = cubeTrans.getMatrix();

    auto cubeNodeID = pBuilder->addNode(cubeNode);

    SceneBuilder::Node quadNode;
    Falcor::Transform quadTrans;
    quadTrans.setTranslation(float3(0, 0, 0));
    quadNode.name = "quad";
    quadNode.transform = quadTrans.getMatrix();

    auto quadNodeID = pBuilder->addNode(quadNode);

    pBuilder->addMeshInstance(cubeNodeID, cubeID);
    pBuilder->addMeshInstance(quadNodeID,quadID);

    auto cam = Falcor::Camera::create("FrontCamera");
    cam->setPosition(float3(1.293189, 0.600636, 0.040573));
    cam->setTarget(float3(0.450143, 0.062795, 0.041240));
    cam->setUpVector(float3(0, 1, 0));
    cam->setFocalLength(35);
    pBuilder->addCamera(cam);

    pLight = Falcor::RectLight::create("RectLight");
    pLight->setIntensity(float3(100,100,100));
    setLight(pLight);
    pBuilder->addLight(pLight);
    mpScene = pBuilder->getScene();*/


    if (mpScene) mShadowPass.mpState->getProgram()->addDefines(mpScene->getSceneDefines());

    mShadowPass.mpVars = GraphicsVars::create(mShadowPass.mpState->getProgram()->getReflector());
    const auto& pReflector = mShadowPass.mpVars->getReflection();
    const auto& pDefaultBlock = pReflector->getDefaultParameterBlock();
    mPerLightCbLoc = pDefaultBlock->getResourceBinding("PerLightCB");

    updateSamplePattern();//default as halton 
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

    createVReusePassResouces();
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
    globalVars["gSTsmCompareSampler"] = mShadowPass.pLinearCmpSampler;
    Sampler::Desc SamplerDesc;
    //SamplerDesc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Linear);
    SamplerDesc.setFilterMode(Sampler::Filter::Point, Sampler::Filter::Point, Sampler::Filter::Point);
    globalVars["gTextureSampler"] = Sampler::create(SamplerDesc);
    csmDataVar.setBlob(mSMData);
}

void SpatioTemporalSM::executeShadowPass(RenderContext* pRenderContext, Texture::SharedPtr pTexture)
{
    
    sampleLightSample();//save in mSMData,used in shader 
    //todo optimize above

    mShadowPass.mpFbo->attachDepthStencilTarget(pTexture);
    mShadowPass.mpState->setFbo(mShadowPass.mpFbo);
    pRenderContext->clearDsv(pTexture->getDSV().get(), 1, 0);

    mShadowPass.mpVars["PerFrameCB"]["gTest"] = 1.0f;//to do delete this

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

    mpScene->rasterize(pRenderContext, mShadowPass.mpState.get(), mShadowPass.mpVars.get());
}

void SpatioTemporalSM::updateLightCamera()
{
    auto getLightCamera = [this]() {//get Light Camera,if not exist ,return default camera
        const auto& Cameras = mpScene->getCameras();
        for (const auto& Camera : Cameras) if (Camera->getName() == "LightCamera") return Camera;
        return Cameras[0];
    };

    mpLightCamera = getLightCamera();
}

float3 SpatioTemporalSM::getAreaLightDir()
{
    assert(mpLight != nullptr);

    float3 AreaLightCenter = float3(0, 0, 0);
    float3 AreaLightDir = float3(0, 0, 1);//use normal as dirction

    float3 AreaLookAtPoint = AreaLightCenter + AreaLightDir;

    float4 AreaLightCenterPosW = mpLight->getData().transMat * float4(AreaLightCenter, 1.0f);
    float4 AreaLightLookAtPointPosw = mpLight->getData().transMat * float4(AreaLookAtPoint, 1.0f);

    float3 AreaLightDirW = normalize(AreaLightLookAtPointPosw.xyz - AreaLightCenterPosW.xyz);//world space dir

    return AreaLightDirW;
}

void SpatioTemporalSM::sampleLightSample()
{
    //sampleWithTargetFixed();

    if (bShowUnjitteredShadowMap) sampleAreaPosW();
    else sampleWithDirectionFixed();
}

void SpatioTemporalSM::sampleWithTargetFixed()
{
    auto getLightCamera = [this]() {//get Light Camera,if not exist ,return default camera
        const auto& Cameras = mpScene->getCameras();
        for (const auto& Camera : Cameras) if (Camera->getName() == "LightCamera") return Camera;
        return Cameras[0];
    };
    mpLightCamera = getLightCamera();

    float2 jitterSample = getJitteredSample();
    float4 SamplePosition = float4(jitterSample, 0, 1);
    
    SamplePosition = mpLight->getData().transMat * SamplePosition;
    std::cout << SamplePosition.x << "," << SamplePosition.y << "," << SamplePosition.z << std::endl;//todo:delele this
    mpLightCamera->setPosition(SamplePosition.xyz);//todo : if need to change look at target
    mSMData.globalMat = mpLightCamera->getViewProjMatrix();
}


void SpatioTemporalSM::sampleWithDirectionFixed()
{
    updateLightCamera();//todo:no need no update every frame

    float3 LightDir = getAreaLightDir();//normalized dir
    float2 jitteredPos = getJitteredSample();
    float4 SamplePos = float4(jitteredPos, 0.f, 1.f);//Local space
    SamplePos = mpLight->getData().transMat * SamplePos;
    float3 LookAtPos = SamplePos.xyz + LightDir;//todo:maybe have error

    mpLightCamera->setPosition(SamplePos.xyz);
    mpLightCamera->setTarget(LookAtPos);

    mSMData.globalMat = mpLightCamera->getViewProjMatrix();//update light matrix
}

void SpatioTemporalSM::sampleAreaPosW()
{
    updateLightCamera();//todo

    float3 EyePosBehindAreaLight = calacEyePosition();
    float4 SamplePosition = float4(EyePosBehindAreaLight, 1);
    SamplePosition = mpLight->getData().transMat * SamplePosition;

    float3 LightDir = getAreaLightDir();//normalized dir
    float3 LookAtPos = SamplePosition.xyz + LightDir;//todo:maybe have error
    mpScene->getAnimations();
    mpLightCamera->setPosition(SamplePosition.xyz);//todo : if need to change look at target
    mpLightCamera->setTarget(LookAtPos);
    mSMData.globalMat = mpLightCamera->getViewProjMatrix();
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

void SpatioTemporalSM::setLight(const Light::SharedConstPtr& pLight)
{
    mpLight = pLight;
}

void SpatioTemporalSM::updateSamplePattern()
{
    mJitterPattern.mpSampleGenerator = createSamplePattern(mJitterPattern.mSamplePattern, mJitterPattern.mSampleCount);
    if (mJitterPattern.mpSampleGenerator) mJitterPattern.mSampleCount = mJitterPattern.mpSampleGenerator->getSampleCount();
}

float2 SpatioTemporalSM::getJitteredSample(bool isScale)
{
    float2 jitter = float2(0.f, 0.f);
    if (mJitterPattern.mpSampleGenerator)
    {
        jitter = mJitterPattern.mpSampleGenerator->next();

        if (isScale) jitter *= mJitterPattern.scale;
    }
    return jitter;
}

void SpatioTemporalSM::createVReusePassResouces()
{
    mVReusePass.mpPass = FullScreenPass::create(kTemporalReusePassfile);
    mVReusePass.mpFbo = Fbo::create();
}

void SpatioTemporalSM::allocatePrevBuffer(const Texture* pTexture)
{
    bool allocate = mVReusePass.mpPrevVisibility == nullptr;
    allocate = allocate || (mVReusePass.mpPrevVisibility->getWidth() != pTexture->getWidth());
    allocate = allocate || (mVReusePass.mpPrevVisibility->getHeight() != pTexture->getHeight());
    allocate = allocate || (mVReusePass.mpPrevVisibility->getDepth() != pTexture->getDepth());
    allocate = allocate || (mVReusePass.mpPrevVisibility->getFormat() != pTexture->getFormat());

    assert(pTexture->getSampleCount() == 1);

    if (allocate) mVReusePass.mpPrevVisibility = Texture::create2D(pTexture->getWidth(), pTexture->getHeight(),
        pTexture->getFormat(), 1, 1, nullptr, Resource::BindFlags::RenderTarget | Resource::BindFlags::ShaderResource);
}

void SpatioTemporalSM::updateBlendWeight()
{
    if (mIterationIndex == 0) return;

    mVContronls.alpha = float(1.0 / mIterationIndex);
    ++mIterationIndex;
}

float3 SpatioTemporalSM::calacEyePosition()
{
    float fovy = mpLightCamera->getFovY();//todo get fovy
    float halfFovy = fovy / 2.0f;

    float maxAreaL = 2.0f;//Local Space

    float dLightCenter2EyePos = maxAreaL / (2.0f * std::tan(halfFovy));

    float3 LightCenter = float3(0, 0, 0);//Local Space
    float3 LightNormal = float3(0, 0, 1);//Local Space
    float3 EyePos = LightCenter - (dLightCenter2EyePos * LightNormal);


    return EyePos;
}

