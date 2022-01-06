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
#include "ShadowMapConstant.slangh"

namespace
{
    const char kDesc[] = "Simple Shadow map pass";

    // output
    const std::string kShadowMapSet = "ShadowMap";
   
    // shader file path
    const std::string kPointGenerationFile = "RenderPasses/SpatioTemporalSM/PointGenerationPass.slang";
    const std::string kShadowPassfile = "RenderPasses/SpatioTemporalSM/ShadowPass.cs.slang";
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
    __createPointGenerationPassResource();
    __createShadowPassResource();
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
    reflector.addOutput(kShadowMapSet, "ShadowMapSet").bindFlags(ResourceBindFlags::UnorderedAccess | Resource::BindFlags::ShaderResource).format(mShadowMapPass.DepthFormat).texture2D(mShadowMapPass.MapSize.x, mShadowMapPass.MapSize.y, 0, 1, mNumShadowMapPerFrame);
    return reflector;
}

void STSM_MultiViewShadowMap::execute(RenderContext* vRenderContext, const RenderData& vRenderData)
{
    if (!mpScene || !mLightInfo.pLight) return;

    __executePointGenerationPass(vRenderContext, vRenderData);
    __executeShadowMapPass(vRenderContext, vRenderData);

    // write internal data
    InternalDictionary& Dict = vRenderData.getDictionary();
    Dict["ShadowMapData"] = mShadowMapPass.ShadowMapData;
}

void STSM_MultiViewShadowMap::renderUI(Gui::Widgets& widget)
{
    if (!mLightInfo.RectLightList.empty())
    {
        widget.dropdown("Current Light", mLightInfo.RectLightList, mLightInfo.CurrentRectLightIndex);
        __updateAreaLight(mLightInfo.CurrentRectLightIndex);
        widget.var("Light Size Scale", mLightInfo.CustomScale, 0.1f, 3.0f, 0.01f);
        mLightInfo.pLight->setScaling(mLightInfo.OriginalScale * mLightInfo.CustomScale);
    }
    else
        widget.text("No Light in Scene");

    widget.checkbox("Jitter Area Light Camera", mVContronls.jitterAreaLightCamera);
    widget.var("Sample Count", mJitterPattern.mSampleCount, 0u, 1000u, 1u);
}

void STSM_MultiViewShadowMap::setScene(RenderContext* pRenderContext, const Scene::SharedPtr& pScene)
{
    mpScene = pScene;
    _ASSERTE(mpScene);
    // find all rect area light
    mLightInfo.pLight = nullptr;
    mLightInfo.RectLightList.clear();
    uint32_t LightNum = mpScene->getLightCount();
    for (uint32_t i = 0; i < LightNum; ++i)
    {
        auto pLight = mpScene->getLight(i);
        if (pLight->getType() == LightType::Rect)
            mLightInfo.RectLightList.emplace_back(Gui::DropdownValue{ i, pLight->getName() });
    }
    _ASSERTE(mLightInfo.RectLightList.size() > 0);
    __updateAreaLight(mLightInfo.RectLightList[0].value);

    // set light camera
    mLightInfo.pCamera = nullptr;
    const auto& CameraSet = mpScene->getCameras();
    for (auto pCamera : CameraSet)
    {
        if (pCamera->getName() == "LightCamera")
        {
            mLightInfo.pCamera = pCamera;
            break;
        }
    }
    _ASSERTE(mLightInfo.pCamera);
    mLightInfo.pCamera->setUpVector(float3(1.0, 0.0, 0.0));
    mLightInfo.pCamera->setAspectRatio((float)mShadowMapPass.MapSize.x / (float)mShadowMapPass.MapSize.y);

    __updateSamplePattern(); //default as halton
    __updatePointGenerationPass();
}

void STSM_MultiViewShadowMap::__createPointGenerationPassResource()
{
    Program::Desc ShaderDesc;
    ShaderDesc.addShaderLibrary(kPointGenerationFile).vsEntry("vsMain").psEntry("psMain");

    GraphicsProgram::SharedPtr pProgram = GraphicsProgram::create(ShaderDesc);

    RasterizerState::Desc RasterDesc;
    RasterDesc.setCullMode(mPointGenerationPass.CullMode);
    RasterizerState::SharedPtr pRasterState = RasterizerState::create(RasterDesc);

    DepthStencilState::Desc DepthStateDesc;
    DepthStateDesc.setDepthEnabled(false);
    DepthStateDesc.setDepthWriteMask(false);
    DepthStencilState::SharedPtr pDepthState = DepthStencilState::create(DepthStateDesc);

    mPointGenerationPass.pState = GraphicsState::create();
    mPointGenerationPass.pState->setProgram(pProgram);
    mPointGenerationPass.pState->setRasterizerState(pRasterState);
    mPointGenerationPass.pState->setDepthStencilState(pDepthState);
    mPointGenerationPass.pState->setBlendState(nullptr);
}

void STSM_MultiViewShadowMap::__createShadowPassResource()
{
    ComputeProgram::SharedPtr pProgram = ComputeProgram::createFromFile(kShadowPassfile, "main");
    mShadowMapPass.pVars = ComputeVars::create(pProgram->getReflector());
    mShadowMapPass.pState = ComputeState::create();
    mShadowMapPass.pState->setProgram(pProgram);
}

void STSM_MultiViewShadowMap::__updatePointGenerationPass()
{
    if (!mpScene || !mLightInfo.pLight || !mLightInfo.pCamera) return;

    // create camera behind light and calculate resolution
    const uint2 ShadowMapSize = mShadowMapPass.MapSize;
    float3 AreaLightCenter = __getAreaLightCenterPos();
    float2 AreaLightSize = __getAreaLightSize();
    float LightSize = std::max(AreaLightSize.x, AreaLightSize.y); // TODO: calculate actual light size
    float FovY = mLightInfo.pCamera->getFovY();
    float FovX = 2 * atan(tan(FovY * 0.5f) * ShadowMapSize.y / ShadowMapSize.x);
    float Fov = std::max(FovX, FovY);
    float Distance = LightSize * 0.5f / tan(Fov * 0.5f); // Point Generation Camera Distance

    float LightCameraNear = mLightInfo.pCamera->getNearPlane();
    float Scaling = ((LightCameraNear + Distance) / LightCameraNear); // Scaling of rasterization resolution
    // FIXME: the calculated scale seems too large, so I put a limitation
    Scaling = std::min(2.0f, Scaling);
    mPointGenerationPass.CoverMapSize = uint2(ShadowMapSize.x * Scaling, ShadowMapSize.y * Scaling);

    Camera::SharedPtr pTempCamera = Camera::create("TempCamera");
    pTempCamera->setPosition(AreaLightCenter - __getAreaLightDir() * Distance);
    pTempCamera->setTarget(AreaLightCenter);
    pTempCamera->setUpVector(mLightInfo.pCamera->getUpVector());
    pTempCamera->setAspectRatio(1.0f); 
    pTempCamera->setFrameHeight(mLightInfo.pCamera->getFrameHeight());
    float FocalLength = fovYToFocalLength(Fov, mLightInfo.pCamera->getFrameHeight());
    pTempCamera->setFocalLength(FocalLength);
    mPointGenerationPass.CoverLightViewProjectMat = pTempCamera->getViewProjMatrix();

    Fbo::Desc FboDesc;
    FboDesc.setDepthStencilTarget(ResourceFormat::D32Float);
    Fbo::SharedPtr pFbo = Fbo::create2D(mPointGenerationPass.CoverMapSize.x, mPointGenerationPass.CoverMapSize.y, FboDesc);

    mPointGenerationPass.pState->setFbo(pFbo);

    auto pProgram = mPointGenerationPass.pState->getProgram();
    pProgram->addDefines(mpScene->getSceneDefines());
    mPointGenerationPass.pVars = GraphicsVars::create(pProgram->getReflector());

    // create append buffer to storage point list
    mPointGenerationPass.pPointAppendBuffer = Buffer::createStructured(pProgram.get(), "PointList", mPointGenerationPass.MaxPointNum);
    mPointGenerationPass.pStageCounterBuffer = Buffer::create(sizeof(uint32_t), ResourceBindFlags::ShaderResource);

    // need to regenerate point list
    mPointGenerationPass.Regenerate = true;
}

float3 STSM_MultiViewShadowMap::__getAreaLightDir()
{
    assert(mLightInfo.pLight != nullptr);

    float3 AreaLightDir = float3(0, 0, 1);//use normal as direction
    // normal is axis-aligned, so no need to construct normal transform matrix
    float3 AreaLightDirW = normalize(mLightInfo.pLight->getData().transMat * float4(AreaLightDir, 0.0f)).xyz;

    return AreaLightDirW;
}

float3 STSM_MultiViewShadowMap::__getAreaLightCenterPos()
{
    assert(mLightInfo.pLight != nullptr);

    float3 AreaLightCenter = float3(0, 0, 0);
    float4 AreaLightCenterPosH = mLightInfo.pLight->getData().transMat * float4(AreaLightCenter, 1.0f);
    float3 AreaLightCenterPosW = AreaLightCenterPosH.xyz * (1.0f / AreaLightCenterPosH.w);

    return AreaLightCenterPosW;
}

float2 STSM_MultiViewShadowMap::__getAreaLightSize()
{
    assert(mLightInfo.pLight != nullptr);

    float3 XMin = float3(-1, 0, 0);
    float3 XMax = float3(1, 0, 0);
    float3 YMin = float3(0, -1, 0);
    float3 YMax = float3(0, 1, 0);

    float4x4 LightTransform = mLightInfo.pLight->getData().transMat;
    float4 XMinH = LightTransform * float4(XMin, 1.0f);
    float4 XMaxH = LightTransform * float4(XMax, 1.0f);
    float4 YMinH = LightTransform * float4(YMin, 1.0f);
    float4 YMaxH = LightTransform * float4(YMax, 1.0f);

    float3 XMinW = XMinH.xyz * (1.0f / XMinH.w);
    float3 XMaxW = XMaxH.xyz * (1.0f / XMaxH.w);
    float3 YMinW = YMinH.xyz * (1.0f / YMinH.w);
    float3 YMaxW = YMaxH.xyz * (1.0f / YMaxH.w);

    float Width = distance(XMinH, XMaxH);
    float Height = distance(YMinW, YMaxW);

    return float2(Width, Height);
}

void STSM_MultiViewShadowMap::__sampleLight()
{
    if (!mVContronls.jitterAreaLightCamera) __sampleAreaPosW();
    else __sampleWithDirectionFixed();
}

void STSM_MultiViewShadowMap::__sampleWithDirectionFixed()
{
    _ASSERTE(mLightInfo.pCamera);

    float3 LightDir = __getAreaLightDir();//normalized dir

    for (uint i = 0; i < mNumShadowMapPerFrame; ++i)
    {
        float2 jitteredPos = __getJitteredSample();
        float4 SamplePos = float4(jitteredPos, 0.f, 1.f);//Local space
        SamplePos = mLightInfo.pLight->getData().transMat * SamplePos;
        float3 LookAtPos = SamplePos.xyz + LightDir;//todo:maybe have error

        mLightInfo.pCamera->setPosition(SamplePos.xyz);
        mLightInfo.pCamera->setTarget(LookAtPos);

        glm::mat4 VP = mLightInfo.pCamera->getViewProjMatrix();
        mShadowMapPass.ShadowMapData.allGlobalMat[i] = VP;
    }
}

void STSM_MultiViewShadowMap::__sampleAreaPosW()
{
    _ASSERTE(mLightInfo.pCamera);

    float3 EyePosBehindAreaLight = __calcEyePosition();
    float4 SamplePosition = float4(EyePosBehindAreaLight, 1);
    SamplePosition = mLightInfo.pLight->getData().transMat * SamplePosition;

    float3 LightDir = __getAreaLightDir();//normalized dir
    float3 LookAtPos = SamplePosition.xyz + LightDir;//todo:maybe have error

    mLightInfo.pCamera->setPosition(SamplePosition.xyz);//todo : if need to change look at target
    mLightInfo.pCamera->setTarget(LookAtPos);

    for (uint i = 0; i < mNumShadowMapPerFrame; ++i)
    {
        mShadowMapPass.ShadowMapData.allGlobalMat[i] = mLightInfo.pCamera->getViewProjMatrix();
    }
}

void STSM_MultiViewShadowMap::__updateSamplePattern()
{
    mJitterPattern.mpSampleGenerator = createSamplePattern(mJitterPattern.mSamplePattern, mJitterPattern.mSampleCount);
    if (mJitterPattern.mpSampleGenerator) mJitterPattern.mSampleCount = mJitterPattern.mpSampleGenerator->getSampleCount();
}

float2 STSM_MultiViewShadowMap::__getJitteredSample(bool isScale)
{
    float2 jitter = float2(0.f, 0.f);
    if (mJitterPattern.mpSampleGenerator)
    {
        jitter = mJitterPattern.mpSampleGenerator->next(); 

        if (isScale) jitter *= mJitterPattern.scale;
    }
    return jitter;
}

float3 STSM_MultiViewShadowMap::__calcEyePosition()
{
    float fovy = mLightInfo.pCamera->getFovY();//todo get fovy
    float halfFovy = fovy / 2.0f;

    float maxAreaL = 2.0f;//Local Space

    float dLightCenter2EyePos = maxAreaL / (2.0f * std::tan(halfFovy));

    float3 LightCenter = float3(0, 0, 0);//Local Space
    float3 LightNormal = float3(0, 0, 1);//Local Space
    float3 EyePos = LightCenter - (dLightCenter2EyePos * LightNormal);

    return EyePos;
}

void STSM_MultiViewShadowMap::__executePointGenerationPass(RenderContext* vRenderContext, const RenderData& vRenderData)
{
    if (!mpScene) return;
    if (mPointGenerationPass.Regenerate)
    {
        //mPointGenerationPass.Regenerate = false;

        auto pCounterBuffer = mPointGenerationPass.pPointAppendBuffer->getUAVCounter();
        pCounterBuffer->setElement(0, (uint32_t)0);

        /*auto pFbo = mPointGenerationPass.pState->getFbo();
        auto pDebug = Texture::create2D(mPointGenerationPass.CoverMapSize.x, mPointGenerationPass.CoverMapSize.y, ResourceFormat::RGBA32Float, 1, 1, nullptr, ResourceBindFlags::RenderTarget);
        pFbo->attachColorTarget(pDebug, 0);
        vRenderContext->clearFbo(pFbo.get(), float4(0.0, 0.0, 0.0, 0.0), 0.0, 0);*/

        mPointGenerationPass.pVars["PointList"] = mPointGenerationPass.pPointAppendBuffer;
        mPointGenerationPass.pVars["PerFrameCB"]["gViewProjectMat"] = mPointGenerationPass.CoverLightViewProjectMat;

        mpScene->rasterize(vRenderContext, mPointGenerationPass.pState.get(), mPointGenerationPass.pVars.get(), RasterizerState::CullMode::None);

        vRenderContext->copyBufferRegion(mPointGenerationPass.pStageCounterBuffer.get(), 0, pCounterBuffer.get(), 0, sizeof(uint32_t));

        // FIXME: delete this point list size logging
        uint32_t* pNum = (uint32_t*)mPointGenerationPass.pStageCounterBuffer->map(Buffer::MapType::Read);
        mPointGenerationPass.CurPointNum = *pNum;
        std::cout << "Point List Size: " << *pNum << "\n";
        mPointGenerationPass.pStageCounterBuffer->unmap();
    }
}

void STSM_MultiViewShadowMap::__executeShadowMapPass(RenderContext* vRenderContext, const RenderData& vRenderData)
{
    // clear
    const auto& pShadowMapSet = vRenderData[kShadowMapSet]->asTexture();
    vRenderContext->clearUAV(pShadowMapSet->getUAV().get(), uint4(0, 0, 0, 0));

    // update
    __sampleLight();
    uint32_t NumGroupXY = div_round_up((int)mPointGenerationPass.CurPointNum, _SHADOW_MAP_SHADER_THREAD_NUM_X * _SHADOW_MAP_SHADER_THREAD_NUM_Y * _SHADOW_MAP_SHADER_POINT_PER_THREAD);
    uint32_t NumGroupX = uint32_t(round(sqrt(NumGroupXY)));
    uint32_t NumGroupY = div_round_up(NumGroupXY, NumGroupX);
    uint32_t NumGroupZ = div_round_up((int)mNumShadowMapPerFrame, _SHADOW_MAP_SHADER_THREAD_NUM_Z * _SHADOW_MAP_SHADER_MAP_PER_THREAD);
    uint3 DispatchDim = uint3(NumGroupX, NumGroupY, NumGroupZ);

    mShadowMapPass.pVars["PerFrameCB"]["gShadowMapData"].setBlob(mShadowMapPass.ShadowMapData);
    mShadowMapPass.pVars["PerFrameCB"]["gDispatchDim"] = DispatchDim;
    auto pCounterBuffer = mPointGenerationPass.pPointAppendBuffer->getUAVCounter();
    mShadowMapPass.pVars->setBuffer("gNumPointBuffer", mPointGenerationPass.pStageCounterBuffer);
    mShadowMapPass.pVars->setBuffer("gPointList", mPointGenerationPass.pPointAppendBuffer);
    mShadowMapPass.pVars->setTexture("gOutputShadowMap", pShadowMapSet);

    // execute
    const std::string EventName = "Render Shadow Maps By Compute Shader";
    Profiler::instance().startEvent(EventName);
    vRenderContext->dispatch(mShadowMapPass.pState.get(), mShadowMapPass.pVars.get(), DispatchDim);
    Profiler::instance().endEvent(EventName);
}

void STSM_MultiViewShadowMap::__updateAreaLight(uint vIndex)
{
    mLightInfo.CurrentRectLightIndex = vIndex;
    AnalyticAreaLight::SharedPtr pNewLight = std::dynamic_pointer_cast<AnalyticAreaLight>(mpScene->getLight(vIndex));

    if (pNewLight == mLightInfo.pLight) return;
    mLightInfo.pLight = pNewLight;
    mLightInfo.OriginalScale = pNewLight->getScaling();
}
