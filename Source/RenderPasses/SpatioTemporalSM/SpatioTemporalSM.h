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

using namespace Falcor;

class SpatioTemporalSM : public RenderPass
{
public:
    using SharedPtr = std::shared_ptr<SpatioTemporalSM>;

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
    virtual void execute(RenderContext* pRenderContext, const RenderData& renderData) override;
    virtual void renderUI(Gui::Widgets& widget) override;
    virtual void setScene(RenderContext* pRenderContext, const Scene::SharedPtr& pScene) override;
    virtual bool onMouseEvent(const MouseEvent& mouseEvent) override { return false; }
    virtual bool onKeyEvent(const KeyboardEvent& keyEvent) override { return false; }

private:
    SpatioTemporalSM();

    void setCullMode(RasterizerState::CullMode cullMode) { mShadowPass.mCullMode = cullMode; }
    Scene::SharedPtr mpScene;

    Light::SharedConstPtr mpLight; //not use
    Camera::SharedPtr mpLightCamera;
    glm::mat4 mlightProjView;

    uint2 mMapSize = uint2(2048, 2048);

    float mTest = 1.0;
    StsmData mSMData;

    void createShadowPassResource();
    void createVisibilityPassResource();

    // Set shadow map generation parameters into a program.
    void setDataIntoVars(ShaderVar const& globalVars, ShaderVar const& csmDataVar);
    void executeShadowPass(RenderContext* pRenderContext, Texture::SharedPtr pTexture);

    struct 
    {
        float2 mapSize;
        float fboAspectRatio;
        Fbo::SharedPtr mpFbo;
        GraphicsState::SharedPtr mpState;
        GraphicsVars::SharedPtr mpVars;
        RasterizerState::CullMode mCullMode = RasterizerState::CullMode::Back;
        ResourceFormat mDepthFormat = ResourceFormat::D32Float;
        Sampler::SharedPtr pLinearCmpSampler;
        Sampler::SharedPtr pPointCmpSampler;
    }mShadowPass;

    ProgramReflection::BindLocation mPerLightCbLoc;

    //compute Visibility pass
    void setupVisibilityPassFbo(const Texture::SharedPtr& pVisBuffer);
 
    struct
    {
        FullScreenPass::SharedPtr pPass;
        Fbo::SharedPtr pFbo;
        UniformShaderVarOffset mPassDataOffset;
    }mVisibilityPass;

    struct
    {
        //This is effectively a bool, but bool only takes up 1 byte which messes up setBlob
        uint32_t shouldVisualizeCascades = 0u;
        int3 padding;
        glm::mat4 camInvViewProj;
        uint2 screenDim = { 0, 0 };
        uint32_t mapBitsPerChannel = 32;
    } mVisibilityPassData;

    int mPcfRadius = 5;

    void calcLightViewInfo(const Camera* pCamera);//not use now
    void setLight(const Light::SharedConstPtr& pLight);//not use now

    bool isFirstFrame = true;

    //random sample pattern
    struct 
    {
        uint32_t mSampleCount = 16;  //todo change from ui 
        SamplePattern mSamplePattern = SamplePattern::Halton;  //todo
        CPUSampleGenerator::SharedPtr mpSampleGenerator;
        float2 scale = float2(2.0f, 2.0f);//this is for halton [-0.5,0.5) => [-1.0,1.0)
    }mJitterPattern;
    void updateSamplePattern();
    float2 getJitteredSample(bool isScale = true);

    //temproal blending pass
    struct  
    {
        FullScreenPass::SharedPtr mpPass;
        Texture::SharedPtr mpPrevVisibility;
        Fbo::SharedPtr mpFbo;
    }mVReusePass;

    struct 
    {
        float alpha = 0.02f;
    }mVContronls;

    void createVReusePassResouces();
    void allocatePrevBuffer(const Texture* pTexture);

};
