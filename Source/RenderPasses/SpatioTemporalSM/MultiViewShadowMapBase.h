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
#include "ShadowMapDefines.h"
#include "Falcor.h"
#include "FalcorExperimental.h"
#include "ShadowMapConstant.slangh"
#include "CSampleGenerator.h"

using namespace Falcor;

class STSM_MultiViewShadowMapBase : public RenderPass
{
public:
    using SharedPtr = std::shared_ptr<STSM_MultiViewShadowMapBase>;

    virtual Dictionary getScriptingDictionary() override;
    RenderPassReflection reflect(const CompileData& compileData);
    virtual void execute(RenderContext* vRenderContext, const RenderData& vRenderData) override;
    virtual void renderUI(Gui::Widgets& widget) override;
    virtual void setScene(RenderContext* pRenderContext, const Scene::SharedPtr& pScene) override;
    virtual bool onMouseEvent(const MouseEvent& mouseEvent) override { return false; }
    virtual bool onKeyEvent(const KeyboardEvent& keyEvent) override { return false; }

protected:
    STSM_MultiViewShadowMapBase();

    Scene::SharedPtr mpScene;
    static std::string mKeyShadowMapSet;

    struct
    {
        SShadowMapData ShadowMapData;
    } mShadowMapInfo;

    struct
    {
        Gui::DropdownList RectLightList;
        uint32_t CurrentRectLightIndex = 0;
        RectLight::SharedPtr pLight;
        Camera::SharedPtr pCamera;
        float3 OriginalScale = float3(1.0f);
        float CustomScale = 1.0f;
    } mLightInfo;

    //random sample pattern
    struct
    {
        uint32_t mSampleCount = (uint)(_MAX_SHADOW_MAP_NUM * 4);
        CSampleGenerator::ESamplePattern mSamplePattern = CSampleGenerator::ESamplePattern::Stratitied;
        std::shared_ptr<CSampleGenerator> pSampleGenerator = nullptr;
    } mJitterPattern;
    void __initSamplePattern();

    struct
    {
        bool jitterAreaLightCamera = true;
    } mVContronls;

    void __updateAreaLight(uint vIndex);
    void __sampleLight();
    void __sampleWithDirectionFixed();
    void __sampleAreaPosW();
    float3 __calcEyePosition();
};
