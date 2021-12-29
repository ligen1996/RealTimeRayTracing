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
#include "MultiViewShadowMap.h"
#include "Utils/Sampling/SampleGenerator.h"

namespace
{
    const char kDesc[] = "Simple Shadow map pass";

    // input
    const std::string kDepth = "Depth";

    // internal parameters
    const std::string kMapSize = "mapSize";
    const std::string kDepthFormat = "depthFormat";
    const std::string kShadowMapSet = "ShadowMap";

    // output
    const std::string kVisibility = "Visibility";
    const std::string kDebug = "Debug";
   
    // shader file path
    const std::string kShadowPassfile = "RenderPasses/SpatioTemporalSM/ShadowPass.ps.slang";
    const std::string kVisibilityPassfile = "RenderPasses/SpatioTemporalSM/VisibilityPass.ps.slang";
}

static CPUSampleGenerator::SharedPtr createSamplePattern(STSM_MultiViewShadowMap::SamplePattern type, uint32_t sampleCount)
{
    switch (type)
    {
    case STSM_MultiViewShadowMap::SamplePattern::Center: return nullptr;
    case STSM_MultiViewShadowMap::SamplePattern::DirectX: return DxSamplePattern::create(sampleCount);
    case STSM_MultiViewShadowMap::SamplePattern::Halton: return HaltonSamplePattern::create(sampleCount);
    case STSM_MultiViewShadowMap::SamplePattern::Stratitied: return StratifiedSamplePattern::create(sampleCount);
    default:
        should_not_get_here();
        return nullptr;
    }
}

STSM_MultiViewShadowMap::STSM_MultiViewShadowMap()
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
}

STSM_MultiViewShadowMap::SharedPtr STSM_MultiViewShadowMap::create(RenderContext* pRenderContext, const Dictionary& dict)
{
    return SharedPtr(new STSM_MultiViewShadowMap());
}

std::string STSM_MultiViewShadowMap::getDesc() { return kDesc; }

Dictionary STSM_MultiViewShadowMap::getScriptingDictionary()
{
    return Dictionary();
}

RenderPassReflection STSM_MultiViewShadowMap::reflect(const CompileData& compileData)
{
    // Define the required resources here
    RenderPassReflection reflector;
    reflector.addInput(kDepth, "Depth");
    reflector.addInternal(kShadowMapSet, "ShadowMapSet").bindFlags(Resource::BindFlags::DepthStencil | Resource::BindFlags::ShaderResource).format(mShadowPass.mDepthFormat).texture2D(mMapSize.x, mMapSize.y, 0, 1, mNumShadowMapPerFrame);
    reflector.addOutput(kVisibility, "Visibility").bindFlags(ResourceBindFlags::RenderTarget | Resource::BindFlags::ShaderResource).format(ResourceFormat::RGBA32Float).texture2D(0, 0);
    reflector.addOutput(kDebug, "Debug").bindFlags(ResourceBindFlags::RenderTarget | Resource::BindFlags::ShaderResource).format(ResourceFormat::RGBA32Float).texture2D(0, 0);
    return reflector;
}

void STSM_MultiViewShadowMap::compile(RenderContext* pContext, const CompileData& compileData)
{
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

void STSM_MultiViewShadowMap::execute(RenderContext* vRenderContext, const RenderData& vRenderData)
{
    if (!mpScene || !mpLight) return;

    __executeShadowPass(vRenderContext, vRenderData);
    __executeVisibilityPass(vRenderContext, vRenderData);
    mShadowPass.Time++;
}

void STSM_MultiViewShadowMap::renderUI(Gui::Widgets& widget)
{
    if (!mRectLightList.empty())
        widget.dropdown("Current Light", mRectLightList, mCurrentRectLightIndex);
    else
        widget.text("No Light in Scene");
    widget.checkbox("Jitter Area Light Camera", mVContronls.jitterAreaLightCamera);
    widget.var("Depth Bias", mSMData.depthBias, 0.000f, 0.1f, 0.0005f);
    widget.var("Sample Count", mJitterPattern.mSampleCount, 0u, 1000u, 1u);
    widget.var("PCF Radius", mPcfRadius, 0, 10, 1);
    widget.separator();
    widget.checkbox("Randomly Select Shadow Map", mVContronls.randomSelection);
    if (mVContronls.randomSelection)
    {
        widget.indent(20.0f);
        widget.var("Select Number", mVContronls.selectNum, 1u, mNumShadowMapPerFrame, 1u);
        widget.indent(-20.0f);
    }
}

void STSM_MultiViewShadowMap::setScene(RenderContext* pRenderContext, const Scene::SharedPtr& pScene)
{
    mpScene = pScene;
    _ASSERTE(mpScene);
    // find first area light
    //mpLight = nullptr;
    uint32_t LightNum = mpScene->getLightCount();
    for (uint32_t i = 0; i < LightNum; ++i)
    {
        auto pLight = mpScene->getLight(i);
        if (pLight->getType() == LightType::Rect)
            mRectLightList.emplace_back(Gui::DropdownValue{ i, pLight->getName() });
    }
    _ASSERTE(mRectLightList.size() > 0);
    mCurrentRectLightIndex = mRectLightList[0].value;
    mpLight = mpScene->getLight(mCurrentRectLightIndex);

    mShadowPass.mpState->getProgram()->addDefines(mpScene->getSceneDefines());
    mShadowPass.mpVars = GraphicsVars::create(mShadowPass.mpState->getProgram()->getReflector());
    const auto& pReflector = mShadowPass.mpVars->getReflection();
    const auto& pDefaultBlock = pReflector->getDefaultParameterBlock();
    mPerLightCbLoc = pDefaultBlock->getResourceBinding("PerLightCB");

    updateSamplePattern(); //default as halton 
}

void STSM_MultiViewShadowMap::createShadowPassResource()
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

void STSM_MultiViewShadowMap::createVisibilityPassResource()
{
    _ASSERTE(mNumShadowMapPerFrame <= _MAX_SHADOW_MAP_NUM);

    Program::DefineList Defines;
    Defines.add("SAMPLE_GENERATOR_TYPE", std::to_string(SAMPLE_GENERATOR_UNIFORM));

    mVisibilityPass.pFbo = Fbo::create();
    mVisibilityPass.pPass = FullScreenPass::create(kVisibilityPassfile, Defines);
    mVisibilityPass.mPassDataOffset = mVisibilityPass.pPass->getVars()->getParameterBlock("PerFrameCB")->getVariableOffset("gPass");
}

void STSM_MultiViewShadowMap::updateVisibilityVars()
{
    auto VisibilityVars = mVisibilityPass.pPass->getVars().getRootVar();//update vars

    VisibilityVars["gSTsmCompareSampler"] = mShadowPass.pLinearCmpSampler;
    VisibilityVars["PerFrameCB"]["gSTsmData"].setBlob(mSMData);
    VisibilityVars["PerFrameCB"]["gTime"] = mShadowPass.Time;
    VisibilityVars["PerFrameCB"]["gRandomSelection"] = mVContronls.randomSelection;
    VisibilityVars["PerFrameCB"]["gSelectNum"] = mVContronls.selectNum;
}

void STSM_MultiViewShadowMap::updateLightCamera()
{
    const auto& Cameras = mpScene->getCameras();
    for (const auto& pCamera : Cameras)
    {
        if (pCamera->getName() == "LightCamera")
        {
            mpLightCamera = pCamera;
            return;
        }
    }
    should_not_get_here();
}

float3 STSM_MultiViewShadowMap::getAreaLightDir()
{
    assert(mpLight != nullptr);

    float3 AreaLightCenter = float3(0, 0, 0);
    float3 AreaLightDir = float3(0, 0, 1);//use normal as direction

    float3 AreaLookAtPoint = AreaLightCenter + AreaLightDir;

    float4 AreaLightCenterPosW = mpLight->getData().transMat * float4(AreaLightCenter, 1.0f);
    float4 AreaLightLookAtPointPosw = mpLight->getData().transMat * float4(AreaLookAtPoint, 1.0f);

    float3 AreaLightDirW = normalize(AreaLightLookAtPointPosw.xyz - AreaLightCenterPosW.xyz);//world space dir

    return AreaLightDirW;
}

void STSM_MultiViewShadowMap::sampleLightSample(uint vIndex)
{
    //sampleWithTargetFixed();

    if (!mVContronls.jitterAreaLightCamera) sampleAreaPosW(vIndex);
    else sampleWithDirectionFixed(vIndex);
}

void STSM_MultiViewShadowMap::sampleWithTargetFixed(uint vIndex)
{
    _ASSERTE(vIndex < mNumShadowMapPerFrame);
    auto getLightCamera = [this]() {//get Light Camera,if not exist ,return default camera
        const auto& Cameras = mpScene->getCameras();
        for (const auto& Camera : Cameras) if (Camera->getName() == "LightCamera") return Camera;
        return Cameras[0];
    };
    mpLightCamera = getLightCamera();

    float2 jitterSample = getJitteredSample();
    float4 SamplePosition = float4(jitterSample, 0, 1);
    
    SamplePosition = mpLight->getData().transMat * SamplePosition;
    mpLightCamera->setPosition(SamplePosition.xyz);//todo : if need to change look at target
    mSMData.globalMat = mpLightCamera->getViewProjMatrix();
    mSMData.allGlobalMat[vIndex] = mpLightCamera->getViewProjMatrix();
}

void STSM_MultiViewShadowMap::sampleWithDirectionFixed(uint vIndex)
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
    mSMData.allGlobalMat[vIndex] = mpLightCamera->getViewProjMatrix();//update light matrix
}

void STSM_MultiViewShadowMap::sampleAreaPosW(uint vIndex)
{
    updateLightCamera();

    float3 EyePosBehindAreaLight = calacEyePosition();
    float4 SamplePosition = float4(EyePosBehindAreaLight, 1);
    SamplePosition = mpLight->getData().transMat * SamplePosition;

    float3 LightDir = getAreaLightDir();//normalized dir
    float3 LookAtPos = SamplePosition.xyz + LightDir;//todo:maybe have error

    mpLightCamera->setPosition(SamplePosition.xyz);//todo : if need to change look at target
    mpLightCamera->setTarget(LookAtPos);
    mSMData.globalMat = mpLightCamera->getViewProjMatrix();
    mSMData.allGlobalMat[vIndex] = mpLightCamera->getViewProjMatrix();
}

void STSM_MultiViewShadowMap::updateSamplePattern()
{
    mJitterPattern.mpSampleGenerator = createSamplePattern(mJitterPattern.mSamplePattern, mJitterPattern.mSampleCount);
    if (mJitterPattern.mpSampleGenerator) mJitterPattern.mSampleCount = mJitterPattern.mpSampleGenerator->getSampleCount();
}

float2 STSM_MultiViewShadowMap::getJitteredSample(bool isScale)
{
    float2 jitter = float2(0.f, 0.f);
    if (mJitterPattern.mpSampleGenerator)
    {
        jitter = mJitterPattern.mpSampleGenerator->next();

        if (isScale) jitter *= mJitterPattern.scale;
    }
    return jitter;
}

float3 STSM_MultiViewShadowMap::calacEyePosition()
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

void STSM_MultiViewShadowMap::__executeShadowPass(RenderContext* vRenderContext, const RenderData& vRenderData)
{
    mpLight = mpScene->getLight(mCurrentRectLightIndex);

    const auto& pShadowMapSet = vRenderData[kShadowMapSet]->asTexture();

    // Shadow pass
    for (uint i = 0; i < mNumShadowMapPerFrame; ++i)
    {
        sampleLightSample(i);//save in mSMData,used in shader 
        //todo optimize above

        mShadowPass.mpFbo->attachDepthStencilTarget(pShadowMapSet, 0, i);
        mShadowPass.mpState->setFbo(mShadowPass.mpFbo);
        vRenderContext->clearDsv(pShadowMapSet->getDSV(0, i).get(), 1, 0);

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

        mpScene->rasterize(vRenderContext, mShadowPass.mpState.get(), mShadowPass.mpVars.get());
    }
}

void STSM_MultiViewShadowMap::__executeVisibilityPass(RenderContext* vRenderContext, const RenderData& vRenderData)
{
    const auto pCamera = mpScene->getCamera().get();
    const auto& pVisibility = vRenderData[kVisibility]->asTexture();
    const auto& pShadowMapSet = vRenderData[kShadowMapSet]->asTexture();
    const auto& pDepth = vRenderData[kDepth]->asTexture();
    const auto& pDebug = vRenderData[kDebug]->asTexture();

    mVisibilityPass.pFbo->attachColorTarget(pVisibility, 0);
    mVisibilityPass.pFbo->attachColorTarget(pDebug, 1);
    vRenderContext->clearFbo(mVisibilityPass.pFbo.get(), float4(1, 0, 0, 0), 1, 0, FboAttachmentType::All);//clear visibility buffer
    updateVisibilityVars();
    mVisibilityPassData.camInvViewProj = pCamera->getInvViewProjMatrix();
    mVisibilityPassData.screenDim = uint2(mVisibilityPass.pFbo->getWidth(), mVisibilityPass.pFbo->getHeight());
    mVisibilityPass.pPass["PerFrameCB"][mVisibilityPass.mPassDataOffset].setBlob(mVisibilityPassData);
    mVisibilityPass.pPass["gShadowMapSet"] = pShadowMapSet;
    mVisibilityPass.pPass["gDepth"] = pDepth;
    mVisibilityPass.pPass["PerFrameCB"]["PcfRadius"] = mPcfRadius;
    mVisibilityPass.pPass->execute(vRenderContext, mVisibilityPass.pFbo); // Render visibility buffer
}
