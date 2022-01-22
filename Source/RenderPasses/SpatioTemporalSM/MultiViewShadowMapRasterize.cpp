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
#include "MultiViewShadowMapRasterize.h"
#include "ShadowMapSelectorDefines.h"

namespace
{
    const char kDesc[] = "Simple Shadow map pass by Rasterization";

    // shader file path
    const std::string kShadowPassfile = "RenderPasses/SpatioTemporalSM/ShadowPass.ps.slang";
}

STSM_MultiViewShadowMapRasterize::STSM_MultiViewShadowMapRasterize()
{
    __createShadowPassResource();
}

RenderPassReflection STSM_MultiViewShadowMapRasterize::reflect(const CompileData& compileData)
{
    return STSM_MultiViewShadowMapBase::reflect(compileData);
}

STSM_MultiViewShadowMapRasterize::SharedPtr STSM_MultiViewShadowMapRasterize::create(RenderContext* pRenderContext, const Dictionary& dict)
{
    return SharedPtr(new STSM_MultiViewShadowMapRasterize());
}

std::string STSM_MultiViewShadowMapRasterize::getDesc() { return kDesc; }

void STSM_MultiViewShadowMapRasterize::execute(RenderContext* vRenderContext, const RenderData& vRenderData)
{
    if (!mpScene || !mLightInfo.pLight) return;

    // check if this pass is chosen
    InternalDictionary& Dict = vRenderData.getDictionary();
    if (!Dict.keyExists("ChosenShadowMapPass")) return;
    EShadowMapGenerationType ChosenPass = Dict["ChosenShadowMapPass"];
    if (ChosenPass != EShadowMapGenerationType::RASTERIZE) return;

    STSM_MultiViewShadowMapBase::execute(vRenderContext, vRenderData);

    __executeShadowPass(vRenderContext, vRenderData);
}

void STSM_MultiViewShadowMapRasterize::renderUI(Gui::Widgets& widget)
{
    STSM_MultiViewShadowMapBase::renderUI(widget);
}

void STSM_MultiViewShadowMapRasterize::setScene(RenderContext* pRenderContext, const Scene::SharedPtr& pScene)
{
    STSM_MultiViewShadowMapBase::setScene(pRenderContext, pScene);
    mShadowPass.pState->getProgram()->addDefines(mpScene->getSceneDefines());
    mShadowPass.pVars = GraphicsVars::create(mShadowPass.pState->getProgram()->getReflector());
}

void STSM_MultiViewShadowMapRasterize::__createShadowPassResource()
{
    Program::Desc desc;
    desc.addShaderLibrary(kShadowPassfile).vsEntry("vsMain").psEntry("psMain");

    Program::DefineList defines;
    defines.add("TEST_ALPHA");
    defines.add("_APPLY_PROJECTION");
    defines.add("_ALPHA_CHANNEL", "a");

    GraphicsProgram::SharedPtr pProgram = GraphicsProgram::create(desc, defines);
    mShadowPass.pState = GraphicsState::create();
    mShadowPass.pState->setProgram(pProgram);

    DepthStencilState::Desc DepthStateDesc;
    DepthStateDesc.setDepthEnabled(true);
    DepthStateDesc.setDepthWriteMask(true);
    DepthStencilState::SharedPtr pDepthState = DepthStencilState::create(DepthStateDesc);
    mShadowPass.pState->setDepthStencilState(pDepthState);

    RasterizerState::Desc rsDesc;
    rsDesc.setDepthClamp(true);
    RasterizerState::SharedPtr rsState = RasterizerState::create(rsDesc);
    mShadowPass.pState->setRasterizerState(rsState);

    Fbo::Desc fboDesc;
    fboDesc.setDepthStencilTarget(ResourceFormat::D32Float);
    mShadowPass.pFbo = Fbo::create2D(gShadowMapSize.x, gShadowMapSize.y, fboDesc); //todo change shadow map size

    mShadowPass.pState->setFbo(mShadowPass.pFbo);
}

void STSM_MultiViewShadowMapRasterize::__executeShadowPass(RenderContext* vRenderContext, const RenderData& vRenderData)
{
    const auto& pShadowMapSet = vRenderData[mKeyShadowMapSet]->asTexture();
    const auto& pIdSet = vRenderData[mKeyIdSet]->asTexture();

    for (uint i = 0; i < gShadowMapNumPerFrame; ++i)
    {
        mShadowPass.pFbo->attachColorTarget(pShadowMapSet, 0, 0, i); 
        mShadowPass.pFbo->attachColorTarget(pIdSet, 1, 0, i);
        vRenderContext->clearFbo(mShadowPass.pFbo.get(), float4(1.0, 0.0, 0.0, 0.0), 1.0f, 0);
        mShadowPass.pVars["PerLightCB"]["gGlobalMat"] = mShadowMapInfo.ShadowMapData.allGlobalMat[i];
        mpScene->rasterize(vRenderContext, mShadowPass.pState.get(), mShadowPass.pVars.get(), mShadowPass.CullMode);
    }
}
