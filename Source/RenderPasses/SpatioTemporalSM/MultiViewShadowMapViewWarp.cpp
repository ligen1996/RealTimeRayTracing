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
#include "MultiViewShadowMapViewWarp.h"
#include "ShadowMapSelectorDefines.h"

namespace
{
    const char kDesc[] = "Simple Shadow map pass by View Warp";

    // internal
    const std::string kInternalInfoSet = "InternalShadowMap";

    // shader file path
    const std::string kPointGenerationFile = "RenderPasses/SpatioTemporalSM/PointGenerationPass.slang";
    const std::string kShadowPassfile = "RenderPasses/SpatioTemporalSM/ShadowPass.cs.slang";
    const std::string kUnpackPassfile = "RenderPasses/SpatioTemporalSM/Unpack.ps.slang";
}

STSM_MultiViewShadowMapViewWarp::STSM_MultiViewShadowMapViewWarp()
{
    __createPointGenerationPassResource();
    __createShadowPassResource();
    __createUnpackPassResource();
}

STSM_MultiViewShadowMapViewWarp::SharedPtr STSM_MultiViewShadowMapViewWarp::create(RenderContext* pRenderContext, const Dictionary& dict)
{
    return SharedPtr(new STSM_MultiViewShadowMapViewWarp());
}

RenderPassReflection STSM_MultiViewShadowMapViewWarp::reflect(const CompileData& compileData)
{
    RenderPassReflection reflector = STSM_MultiViewShadowMapBase::reflect(compileData);
    reflector.addInternal(kInternalInfoSet, "InternalShadowMapSet").bindFlags(Resource::BindFlags::UnorderedAccess | Resource::BindFlags::ShaderResource).format(mShadowMapPass.InternalDepthFormat).texture2D(gShadowMapSize.x, gShadowMapSize.y, 0, 1, _SHADOW_MAP_NUM);
    return reflector;
}

std::string STSM_MultiViewShadowMapViewWarp::getDesc() { return kDesc; }

void STSM_MultiViewShadowMapViewWarp::execute(RenderContext* vRenderContext, const RenderData& vRenderData)
{
    if (!mpScene || !mLightInfo.pLight) return;

    // check if this pass is chosen
    InternalDictionary& Dict = vRenderData.getDictionary();
    if (Dict.keyExists("ChosenShadowMapPass"))
    {
        EShadowMapGenerationType ChosenPass = Dict["ChosenShadowMapPass"];
        if (ChosenPass != EShadowMapGenerationType::VIEW_WARP) return;
    }

    STSM_MultiViewShadowMapBase::execute(vRenderContext, vRenderData);

    __executePointGenerationPass(vRenderContext, vRenderData);
    __executeShadowMapPass(vRenderContext, vRenderData);
    __executeUnpackPass(vRenderContext, vRenderData);
}

void STSM_MultiViewShadowMapViewWarp::renderUI(Gui::Widgets& widget)
{
    STSM_MultiViewShadowMapBase::renderUI(widget);
    widget.separator();
    widget.checkbox("Force Point Regeneration", mVContronls.ForcePointRegeneration);
    widget.checkbox("Use Max Point Count", mVContronls.UseMaxPointCount);

    std::string PointCount = std::to_string(mPointGenerationPass.CurPointNum);
    if (mVContronls.UseMaxPointCount)
        PointCount = std::to_string(mPointGenerationPass.MaxPointNum);

    widget.text("Used Point Count: " + PointCount);
}

void STSM_MultiViewShadowMapViewWarp::setScene(RenderContext* pRenderContext, const Scene::SharedPtr& pScene)
{
    STSM_MultiViewShadowMapBase::setScene(pRenderContext, pScene);
    __updatePointGenerationPass();
}

void STSM_MultiViewShadowMapViewWarp::__createPointGenerationPassResource()
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

void STSM_MultiViewShadowMapViewWarp::__createShadowPassResource()
{
    ComputeProgram::SharedPtr pProgram = ComputeProgram::createFromFile(kShadowPassfile, "main");
    mShadowMapPass.pVars = ComputeVars::create(pProgram->getReflector());
    mShadowMapPass.pState = ComputeState::create();
    mShadowMapPass.pState->setProgram(pProgram);
}

void STSM_MultiViewShadowMapViewWarp::__createUnpackPassResource()
{
    mUnpackPass.pFbo = Fbo::create(); 
    mUnpackPass.pPass = FullScreenPass::create(kUnpackPassfile);
}

void STSM_MultiViewShadowMapViewWarp::__updatePointGenerationPass()
{
    if (!mpScene || !mLightInfo.pLight) return;

    auto pRefCamera = mLightInfo.pCamera ? mLightInfo.pCamera : mpScene->getCamera();

    // create camera behind light and calculate resolution
    const uint2 ShadowMapSize = gShadowMapSize;
    float3 AreaLightCenter = mLightInfo.pLight->getCenter();
    float3 AreaLightDirection = mLightInfo.pLight->getDirection();
    float2 AreaLightSize = mLightInfo.pLight->getSize();
    float LightSize = std::max(AreaLightSize.x, AreaLightSize.y); // TODO: calculate actual light size
    float FovY = mLightInfo.pLight->getOpeningAngle();
    float FovX = 2 * atan(tan(FovY * 0.5f) * ShadowMapSize.y / ShadowMapSize.x);
    float Fov = std::max(FovX, FovY);
    float Distance = LightSize * 0.5f / tan(Fov * 0.5f); // Point Generation Camera Distance

    float LightCameraNear = pRefCamera->getNearPlane();
    float Scaling = ((LightCameraNear + Distance) / LightCameraNear); // Scaling of rasterization resolution
    // FIXME: the calculated scale seems too large, so I put a limitation
    Scaling = std::min(2.0f, Scaling);
    mPointGenerationPass.CoverMapSize = uint2(ShadowMapSize.x * Scaling, ShadowMapSize.y * Scaling);

    // TODO: hard to use helper function in this, as helper auto choose a near by pos, while pos depends on near here
    float3 Up = float3(0, 1, 0);
    if (abs(AreaLightDirection.y) > 0.95) Up = float3(1, 0, 0);

    Camera::SharedPtr pTempCamera = Camera::create("TempCamera");
    pTempCamera->setPosition(AreaLightCenter - AreaLightDirection * Distance);
    pTempCamera->setTarget(AreaLightCenter);
    pTempCamera->setUpVector(Up);
    pTempCamera->setAspectRatio(1.0f); 
    pTempCamera->setFrameHeight(pRefCamera->getFrameHeight());
    float FocalLength = fovYToFocalLength(Fov, pRefCamera->getFrameHeight());
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
    mPointGenerationPass.pPointAndIdAppendBuffer = Buffer::createStructured(pProgram.get(), "PointAndIdList", mPointGenerationPass.MaxPointNum);
    mPointGenerationPass.pStageCounterBuffer = Buffer::create(sizeof(uint32_t), ResourceBindFlags::ShaderResource);

    // need to regenerate point list
    mPointGenerationPass.Regenerate = true;
}

void STSM_MultiViewShadowMapViewWarp::__updatePointCount()
{
    uint32_t* pNum = (uint32_t*)mPointGenerationPass.pStageCounterBuffer->map(Buffer::MapType::Read);
    mPointGenerationPass.CurPointNum = *pNum;
    std::cout << "Point List Count: " << *pNum << "\n";
    mPointGenerationPass.pStageCounterBuffer->unmap();
}

void STSM_MultiViewShadowMapViewWarp::__executePointGenerationPass(RenderContext* vRenderContext, const RenderData& vRenderData)
{
    if (!mpScene) return;
    if (mPointGenerationPass.Regenerate || mVContronls.ForcePointRegeneration)
    {
        mPointGenerationPass.Regenerate = false;

        auto pCounterBuffer = mPointGenerationPass.pPointAndIdAppendBuffer->getUAVCounter();
        pCounterBuffer->setElement(0, (uint32_t)0);

        /*auto pFbo = mPointGenerationPass.pState->getFbo();
        auto pDebug = Texture::create2D(mPointGenerationPass.CoverMapSize.x, mPointGenerationPass.CoverMapSize.y, ResourceFormat::RGBA32Float, 1, 1, nullptr, ResourceBindFlags::RenderTarget);
        pFbo->attachColorTarget(pDebug, 0);
        vRenderContext->clearFbo(pFbo.get(), float4(0.0, 0.0, 0.0, 0.0), 0.0, 0);*/

        mPointGenerationPass.pVars["PointAndIdList"] = mPointGenerationPass.pPointAndIdAppendBuffer;
        mPointGenerationPass.pVars["PerFrameCB"]["gViewProjectMat"] =  mPointGenerationPass.CoverLightViewProjectMat;

        mpScene->rasterize(vRenderContext, mPointGenerationPass.pState.get(), mPointGenerationPass.pVars.get(), RasterizerState::CullMode::None);

        vRenderContext->copyBufferRegion(mPointGenerationPass.pStageCounterBuffer.get(), 0, pCounterBuffer.get(), 0, sizeof(uint32_t));

        if (!mVContronls.UseMaxPointCount)
            __updatePointCount();
    }
}

void STSM_MultiViewShadowMapViewWarp::__executeShadowMapPass(RenderContext* vRenderContext, const RenderData& vRenderData)
{
    const auto& pInternalInfoSet = vRenderData[kInternalInfoSet]->asTexture();
    vRenderContext->clearUAV(pInternalInfoSet->getUAV().get(), uint4(0, 0, 0, 0));

    uint UsedPointNum = mVContronls.UseMaxPointCount ? mPointGenerationPass.MaxPointNum : mPointGenerationPass.CurPointNum;
    if (!UsedPointNum) return;

    uint32_t NumGroupXY = div_round_up((int)UsedPointNum, _SHADOW_MAP_SHADER_THREAD_NUM_X * _SHADOW_MAP_SHADER_THREAD_NUM_Y * _SHADOW_MAP_SHADER_POINT_PER_THREAD);
    uint32_t NumGroupX = uint32_t(round(sqrt(NumGroupXY)));
    uint32_t NumGroupY = div_round_up(NumGroupXY, NumGroupX);
    uint32_t NumGroupZ = div_round_up((int)_SHADOW_MAP_NUM, _SHADOW_MAP_SHADER_THREAD_NUM_Z * _SHADOW_MAP_SHADER_MAP_PER_THREAD);
    uint3 DispatchDim = uint3(NumGroupX, NumGroupY, NumGroupZ);

    mShadowMapPass.pVars["PerFrameCB"]["gShadowMapData"].setBlob(mShadowMapInfo.ShadowMapData);
    mShadowMapPass.pVars["PerFrameCB"]["gDispatchDim"] = DispatchDim;
    auto pCounterBuffer = mPointGenerationPass.pPointAndIdAppendBuffer->getUAVCounter();
    mShadowMapPass.pVars->setBuffer("gNumPointBuffer", mPointGenerationPass.pStageCounterBuffer);
    mShadowMapPass.pVars->setBuffer("gPointAndIdList", mPointGenerationPass.pPointAndIdAppendBuffer);
    mShadowMapPass.pVars->setTexture("gOutputInfo", pInternalInfoSet);

    // execute
    const std::string EventName = "Render Shadow Maps By Compute Shader";
    Profiler::instance().startEvent(EventName);
    vRenderContext->dispatch(mShadowMapPass.pState.get(), mShadowMapPass.pVars.get(), DispatchDim);
    Profiler::instance().endEvent(EventName);
}

void STSM_MultiViewShadowMapViewWarp::__executeUnpackPass(RenderContext* vRenderContext, const RenderData& vRenderData)
{
    PROFILE("Unpack internal info set");
    const auto& pInternalInfoSet = vRenderData[kInternalInfoSet]->asTexture();
    const auto& pShadowMapSet = vRenderData[mKeyShadowMapSet]->asTexture();
    const auto& pIdSet = vRenderData[mKeyIdSet]->asTexture();

    vRenderContext->clearRtv(pShadowMapSet->getRTV().get(), float4(1.0, 0.0, 0.0, 0.0));
    vRenderContext->clearRtv(pIdSet->getRTV().get(), float4(0.0, 0.0, 0.0, 0.0));
    for (uint i = 0; i < _SHADOW_MAP_NUM; ++i)
    {
        mUnpackPass.pFbo->attachColorTarget(pShadowMapSet, 0, 0, i);
        mUnpackPass.pFbo->attachColorTarget(pIdSet, 1, 0, i);
        mUnpackPass.pPass["gInfoSet"] = pInternalInfoSet->asTexture();
        mUnpackPass.pPass["PerFrameCB"]["gIndex"] = uint(i);
        mUnpackPass.pPass["PerFrameCB"]["gMapSize"] = gShadowMapSize;
        mUnpackPass.pPass->execute(vRenderContext, mUnpackPass.pFbo);
    }
}
