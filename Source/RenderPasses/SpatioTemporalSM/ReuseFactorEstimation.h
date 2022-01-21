#pragma once
#include "Falcor.h"
#include "FalcorExperimental.h"

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
    virtual bool onMouseEvent(const MouseEvent& mouseEvent) override { return false; }
    virtual bool onKeyEvent(const KeyboardEvent& keyEvent) override { return false; }

private:
    STSM_ReuseFactorEstimation();

    Texture::SharedPtr mpVariation;
    Texture::SharedPtr mpVarOfVar;

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
        Texture::SharedPtr pTempVariation;
    } mFilterPass;

    struct
    {
        FullScreenPass::SharedPtr pPass;
        Fbo::SharedPtr pFbo;
    } mVarOfVarPass;

    struct
    {
        bool ForceOutputOne = false;
        uint MaxFilterKernelSize = 5u;
        uint TentFilterKernelSize = 13u;
    } mContronls;

    void __executeEstimation(RenderContext* vRenderContext, const RenderData& vRenderData);
    void __executeFilters(RenderContext* vRenderContext, const RenderData& vRenderData);
    void __executeFilter(RenderContext* vRenderContext, const RenderData& vRenderData, uint vFilterType, uint vKernelSize);
    void __executeCalcVarOfVar(RenderContext* vRenderContext, const RenderData& vRenderData);
    void _prepareTempVariationTexture(Texture::SharedPtr vVariation);
};
