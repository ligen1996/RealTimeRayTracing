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
#include "MultiViewShadowMapBase.h"

using namespace Falcor;

class STSM_MultiViewShadowMapViewWarp : public STSM_MultiViewShadowMapBase
{
public:
    using SharedPtr = std::shared_ptr<STSM_MultiViewShadowMapViewWarp>;

    static SharedPtr create(RenderContext* pRenderContext = nullptr, const Dictionary& dict = {});
    virtual RenderPassReflection reflect(const CompileData& compileData) override;
    virtual std::string getDesc() override;
    virtual void execute(RenderContext* vRenderContext, const RenderData& vRenderData) override;
    virtual void renderUI(Gui::Widgets& widget) override;
    virtual void setScene(RenderContext* pRenderContext, const Scene::SharedPtr& pScene) override;

private:
    STSM_MultiViewShadowMapViewWarp();

    struct
    {
        bool Regenerate = true;
        GraphicsState::SharedPtr pState;
        GraphicsVars::SharedPtr pVars;
        RasterizerState::CullMode CullMode = RasterizerState::CullMode::Back;
        Buffer::SharedPtr pPointAppendBuffer;
        Buffer::SharedPtr pPointIdAppendBuffer;
        Buffer::SharedPtr pStageCounterBuffer;
        uint MaxPointNum = 20000000u;
        uint CurPointNum = 0u;
        float4x4 CoverLightViewProjectMat;
        uint2 CoverMapSize;
    } mPointGenerationPass;

    struct
    {
        ComputeState::SharedPtr pState;
        ComputeVars::SharedPtr pVars;
        ResourceFormat InternalDepthFormat = ResourceFormat::R32Uint; // interlock/atomic operation require int/uint
    } mShadowMapPass;

    struct
    {
        Fbo::SharedPtr pFbo;
        FullScreenPass::SharedPtr pPass;
    } mDepthConvertionPass;

    struct
    {
        bool ForcePointRegeneration = false;
        bool UseMaxPointCount = false;
    } mVContronls;

    void __createPointGenerationPassResource();
    void __createShadowPassResource();
    void __createDepthConvertionPassResource();
    void __updatePointGenerationPass();
    void __updatePointCount();

    void __executePointGenerationPass(RenderContext* vRenderContext, const RenderData& vRenderData);
    void __executeShadowMapPass(RenderContext* vRenderContext, const RenderData& vRenderData);
    void __executeDepthConvertionPass(RenderContext* vRenderContext, const RenderData& vRenderData);
};
