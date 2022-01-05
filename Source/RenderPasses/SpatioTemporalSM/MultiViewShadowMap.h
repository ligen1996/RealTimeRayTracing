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
#pragma once
#include "Falcor.h"
#include "FalcorExperimental.h"
#include "STSMData.slang"
#include "ShadowMapConstant.slangh"

using namespace Falcor;

class STSM_MultiViewShadowMap : public RenderPass
{
public:
    using SharedPtr = std::shared_ptr<STSM_MultiViewShadowMap>;

    enum class SamplePattern : uint32_t
    {
        Center,
        DirectX,
        Halton,
        Stratitied,
    };//todo:may add more random pattern

    /** Create a new render pass object.
        \param[in] pRenderContext The render context.
        \param[in] dict Dictionary of serialized parameters.
        \return A new object, or an exception is thrown if creation failed.
    */
    static SharedPtr create(RenderContext* pRenderContext = nullptr, const Dictionary& dict = {});

    virtual std::string getDesc() override;
    virtual Dictionary getScriptingDictionary() override;
    virtual RenderPassReflection reflect(const CompileData& compileData) override;
    virtual void compile(RenderContext* pContext, const CompileData& compileData) override;
    virtual void execute(RenderContext* vRenderContext, const RenderData& vRenderData) override;
    virtual void renderUI(Gui::Widgets& widget) override;
    virtual void setScene(RenderContext* pRenderContext, const Scene::SharedPtr& pScene) override;
    virtual bool onMouseEvent(const MouseEvent& mouseEvent) override { return false; }
    virtual bool onKeyEvent(const KeyboardEvent& keyEvent) override { return false; }

private:
    STSM_MultiViewShadowMap();
    Scene::SharedPtr mpScene;

    struct
    {
        Gui::DropdownList RectLightList;
        uint32_t CurrentRectLightIndex = 0;
        AnalyticAreaLight::SharedPtr pLight;
        Camera::SharedPtr pCamera;
        float3 OriginalScale = float3(1.0f);
        float CustomScale = 1.0f;
    } mLightInfo;
    StsmData mSMData;


    struct
    {
        bool Regenerate = true;
        GraphicsState::SharedPtr pState;
        GraphicsVars::SharedPtr pVars;
        RasterizerState::CullMode CullMode = RasterizerState::CullMode::Back;
        Buffer::SharedPtr pPointAppendBuffer;
        Buffer::SharedPtr pStageCounterBuffer;
        uint MaxPointNum = 20000000u;
        uint CurPointNum = 0u;
        float4x4 CoverLightViewProjectMat;
        uint2 CoverMapSize;
    } mPointGenerationPass;

    struct
    {
        uint2 MapSize = uint2(1024, 1024);
        ComputeState::SharedPtr pState;
        ComputeVars::SharedPtr pVars;
        ResourceFormat DepthFormat = ResourceFormat::R32Uint; // interlock/atomic operation require int/uint
        SShadowMapData ShadowMapData;
        float Time = 0.0;
    } mShadowMapPass;

    struct
    {
        FullScreenPass::SharedPtr pPass;
        Fbo::SharedPtr pFbo;
        UniformShaderVarOffset mPassDataOffset;
        Sampler::SharedPtr pPointCmpSampler;
        Sampler::SharedPtr pLinearCmpSampler;
        int mPcfRadius = 0;
    } mVisibilityPass;

    struct
    {
        //This is effectively a bool, but bool only takes up 1 byte which messes up setBlob
        uint32_t shouldVisualizeCascades = 0u;
        int3 padding;
        glm::mat4 camInvViewProj;
        uint2 screenDim = { 0, 0 };
        uint32_t mapBitsPerChannel = 32;
    } mVisibilityPassData;

    //random sample pattern
    struct
    {
        uint32_t mSampleCount = 64;  //todo change from ui 
        SamplePattern mSamplePattern = SamplePattern::Halton;  //todo
        CPUSampleGenerator::SharedPtr mpSampleGenerator;
        float2 scale = float2(2.0f, 2.0f);//this is for halton [-0.5,0.5) => [-1.0,1.0)
    } mJitterPattern;
    void updateSamplePattern();
    float2 getJitteredSample(bool isScale = true);

    struct
    {
        bool jitterAreaLightCamera = true;
        bool randomSelection = true;
        uint selectNum = 8;
    } mVContronls;

    void createPointGenerationPassResource();
    void createShadowPassResource();
    void createVisibilityPassResource();

    void updatePointGenerationPass();

    // Set shadow map generation parameters into a program.
    void updateVisibilityVars();
    void __executePointGenerationPass(RenderContext* vRenderContext, const RenderData& vRenderData);
    void __executeShadowMapPass(RenderContext* vRenderContext, const RenderData& vRenderData);
    void __executeVisibilityPass(RenderContext* vRenderContext, const RenderData& vRenderData);
    void __updateAreaLight(uint vIndex);

    //sample light sample from area light
    float3 getAreaLightDir();
    float3 getAreaLightCenterPos();
    float2 getAreaLightSize();
    void sampleLight();
    void sampleWithDirectionFixed();
    void sampleAreaPosW();

    uint mNumShadowMapPerFrame = 16;

    float3 calacEyePosition();
};
