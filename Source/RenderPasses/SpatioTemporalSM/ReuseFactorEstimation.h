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

    static void registerScriptBindings(pybind11::module& m);

    uint getMaxFilterKernelSize() { return mControls.MaxFilterKernelSize; }
    void setMaxFilterKernelSize(uint v) { mControls.MaxFilterKernelSize = v; }
    uint getTentFilterKernelSize() { return mControls.TentFilterKernelSize; }
    void setTentFilterKernelSize(uint v) { mControls.TentFilterKernelSize = v; }
    uint getVarOfVarMinFilterKernelSize() { return mControls.VarOfVarMinFilterKernelSize; }
    void setVarOfVarMinFilterKernelSize(uint v) { mControls.VarOfVarMinFilterKernelSize = v; }
    uint getVarOfVarMaxFilterKernelSize() { return mControls.VarOfVarMaxFilterKernelSize; }
    void setVarOfVarMaxFilterKernelSize(uint v) { mControls.VarOfVarMaxFilterKernelSize = v; }
    uint getVarOfVarTentFilterKernelSize() { return mControls.VarOfVarTentFilterKernelSize; }
    void setVarOfVarTentFilterKernelSize(uint v) { mControls.VarOfVarTentFilterKernelSize = v; }
    bool getReuseVariation() { return mControls.ReuseVariation; }
    void setReuseVariation(bool v) { mControls.ReuseVariation = v; }
    float getReuseAlpha() { return mControls.ReuseAlpha; }
    void setReuseAlpha(float v) { mControls.ReuseAlpha = v; }
    float getReuseBeta() { return mControls.ReuseBeta; }
    void setReuseBeta(float v) { mControls.ReuseBeta = v; }
    float getMapMin() { return mControls.MapMin; }
    void setMapMin(float v) { mControls.MapMin = v; }
    float getMapMax() { return mControls.MapMax; }
    void setMapMax(float v) { mControls.MapMax = v; }
    float getReliabilityStrength() { return mControls.ReliabilityStrength; }
    void setReliabilityStrength(float v) { mControls.ReliabilityStrength = v; }
    float getRatiodv() { return mControls.Ratiodv; }
    void setRatiodv(float v) { mControls.Ratiodv = v; }
    float getRatioddv() { return mControls.Ratioddv; }
    void setRatioddv(float v) { mControls.Ratioddv = v; }
    float getDiscardByPositionStrength() { return mControls.DiscardByPositionStrength; }
    void setDiscardByPositionStrength(float v) { mControls.DiscardByPositionStrength = v; }
    float getDiscardByNormalStrength() { return mControls.DiscardByNormalStrength; }
    void setDiscardByNormalStrength(float v) { mControls.DiscardByNormalStrength = v; }
    bool getUseAdaptiveAlpha() { return mControls.UseAdaptiveAlpha; }
    void setUseAdaptiveAlpha(bool v) { mControls.UseAdaptiveAlpha = v; }

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
