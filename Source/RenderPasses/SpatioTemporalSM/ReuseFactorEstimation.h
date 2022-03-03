#pragma once
#include "Falcor.h"
#include "FalcorExperimental.h"
#include "Reuse.h"

using namespace Falcor;

class STSM_ReuseFactorEstimation : public RenderPass
{
public:
    using SharedPtr = std::shared_ptr<STSM_ReuseFactorEstimation>;

    static SharedPtr create(RenderContext* pRenderContext = nullptr, const Dictionary& dict = {});
    virtual std::string getDesc() override;
    virtual Dictionary getScriptingDictionary() override;
    virtual RenderPassReflection reflect(const CompileData& compileData) override;
    virtual void execute(RenderContext* vRenderContext, const RenderData& vRenderData) override;
    virtual void renderUI(Gui::Widgets& widget) override;
    virtual void setScene(RenderContext* pRenderContext, const Scene::SharedPtr& pScene) override;
    virtual bool onMouseEvent(const MouseEvent& mouseEvent) override { return false; }
    virtual bool onKeyEvent(const KeyboardEvent& keyEvent) override { return false; }

private:
    STSM_ReuseFactorEstimation();

    Scene::SharedPtr mpScene;
    Texture::SharedPtr mpVariation;
    Texture::SharedPtr mpVarOfVar;
    Sampler::SharedPtr mpSamplerLinear;
    static Gui::DropdownList mReuseSampleTypeList;

    struct
    {
        FullScreenPass::SharedPtr pPass;
        Fbo::SharedPtr pFbo;
        Texture::SharedPtr pPrevVisibility;
    } mEstimationPass;

    struct
    {
        FullScreenPass::SharedPtr pPass;
        Fbo::SharedPtr pFbo;
        Texture::SharedPtr pTempValue;
    } mFilterPass;

    struct
    {
        FullScreenPass::SharedPtr pPass;
        Fbo::SharedPtr pFbo;
        Texture::SharedPtr pTempValue;
    } mMapPass;

    struct
    {
        FullScreenPass::SharedPtr pPass;
        Fbo::SharedPtr pFbo;
    } mVarOfVarPass;

    struct
    {
        FullScreenPass::SharedPtr pPass;
        Fbo::SharedPtr pFbo;
    } mFixedAlphaReusePass;

    struct
    {
        FullScreenPass::SharedPtr pPass;
        Fbo::SharedPtr pFbo;
    } mAdaptiveAlphaReusePass;

    struct
    {
        EReuseSampleType ReuseSampleType = EReuseSampleType::BILINEAR;
        bool ForceOutputOne = false;
        uint MaxFilterKernelSize = 5u;
        uint TentFilterKernelSize = 19u;
        uint VarOfVarMinFilterKernelSize = 21u;
        uint VarOfVarMaxFilterKernelSize = 3u;
        uint VarOfVarTentFilterKernelSize = 31u;
        bool ReuseVariation = true;
        float ReuseAlpha = 0.2f;
        float ReuseBeta = 0.3f;
        float MapMin = 0.f;
        float MapMax = 1.f;
        float ReliabilityStrength = 1.f;
        float Ratiodv = 0.2f;
        float Ratioddv = 1.0f;
        float DiscardByPositionStrength = 1.0f;
        float DiscardByNormalStrength = 1.0f;
        bool UseAdaptiveAlpha = true;
    } mControls;

    void __executeEstimation(RenderContext* vRenderContext, const RenderData& vRenderData);
    void __executeVariationFilters(RenderContext* vRenderContext, const RenderData& vRenderData);
    void __executeFilter(RenderContext* vRenderContext, const RenderData& vRenderData, Texture::SharedPtr vTarget, uint vFilterType, uint vKernelSize);
    void __executeMap(RenderContext* vRenderContext, const RenderData& vRenderData, Texture::SharedPtr vTarget, uint vMapType, float vParam1, float vParam2);
    void __executeCalcVarOfVar(RenderContext* vRenderContext, const RenderData& vRenderData);
    void __executeFixedAlphaReuse(RenderContext* vRenderContext, const RenderData& vRenderData, Texture::SharedPtr vPrev, Texture::SharedPtr vCur, Texture::SharedPtr vTarget, float vAlpha);
    void __loadVariationTextures(const RenderData& vRenderData, Texture::SharedPtr& voVariation, Texture::SharedPtr& voVarOfVar);
    void __executeAdaptiveAlphaReuse(RenderContext* vRenderContext, const RenderData& vRenderData, Texture::SharedPtr vPrev, Texture::SharedPtr vCur, Texture::SharedPtr vTarget);
    void _prepareTexture(Texture::SharedPtr vRefTex, Texture::SharedPtr& voTexTarget);

    void __loadParams(const pybind11::dict& Dict);
    pybind11::dict __getParams();
};
