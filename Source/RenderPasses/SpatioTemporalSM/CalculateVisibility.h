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
#include "ShadowMapConstant.slangh"

using namespace Falcor;

class STSM_CalculateVisibility : public RenderPass
{
public:
    using SharedPtr = std::shared_ptr<STSM_CalculateVisibility>;

    static SharedPtr create(RenderContext* pRenderContext = nullptr, const Dictionary& dict = {});
    virtual std::string getDesc() override;
    virtual Dictionary getScriptingDictionary() override;
    virtual RenderPassReflection reflect(const CompileData& compileData) override;
    virtual void execute(RenderContext* vRenderContext, const RenderData& vRenderData) override;
    virtual void renderUI(Gui::Widgets& widget) override;
    virtual void setScene(RenderContext* pRenderContext, const Scene::SharedPtr& pScene) override;
    virtual bool onMouseEvent(const MouseEvent& mouseEvent) override { return false; }
    virtual bool onKeyEvent(const KeyboardEvent& keyEvent) override { return false; }

    static void registerScriptBindings(pybind11::module& m);

    bool getRandomSelection() { return mContronls.RandomSelection; }
    void setRandomSelection(bool v) { mContronls.RandomSelection = v; }
    uint getSelectNum() { return mContronls.SelectNum; }
    void setSelectNum(uint v) { mContronls.SelectNum = v; }

private:
    STSM_CalculateVisibility();

    struct
    {
        FullScreenPass::SharedPtr pPass;
        Fbo::SharedPtr pFbo;
        Sampler::SharedPtr pPointCmpSampler;
        Sampler::SharedPtr pLinearCmpSampler;
        SShadowMapData ShadowMapData;
        float Time = 0.0;
        Texture::SharedPtr pLightTexture = Texture::createFromFile("../Data/Texture/1.png", false, false);
        float mLightAverageIntensity = 0.5f;
    } mVisibilityPass;

    struct
    {
        float DepthBias = 0.00015f;
        int PcfRadius = 0;
        bool RandomSelection = true;
        uint SelectNum = 8u;
        float TimeScale = 100.0f;
    } mContronls;

    bool __loadPassInternalData(const RenderData& vRenderData);
    Scene::SharedPtr mpScene;
};
