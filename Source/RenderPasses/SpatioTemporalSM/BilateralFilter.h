#pragma once
#include "Falcor.h"
#include "FalcorExperimental.h"

using namespace Falcor;

class STSM_BilateralFilter : public RenderPass
{
public:
    using SharedPtr = std::shared_ptr<STSM_BilateralFilter>;

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
    virtual bool onMouseEvent(const MouseEvent& mouseEvent) override { return false; }
    virtual bool onKeyEvent(const KeyboardEvent& keyEvent) override { return false; }

    enum class EFilterDirection : uint
    {
        X = 0u,
        Y,
        BOTH
    };

    enum class EMethod : uint
    {
        PIXEL_SHADER,
        COMPUTE_SHADER
    };

    static std::string toString(EFilterDirection vType);
    static std::string toString(EMethod vMethod);

private:
    STSM_BilateralFilter();

    static Gui::DropdownList mDirectionList;
    static Gui::DropdownList mMethodList;

    struct  
    {
        FullScreenPass::SharedPtr pPass;
        Fbo::SharedPtr pFbo;
        Texture::SharedPtr pIterStageTexture;
        Texture::SharedPtr pTempStageTexture;
    } mPixelFilterPass;

    struct
    {
        ComputeState::SharedPtr pState;
        ComputeVars::SharedPtr pVars;
        Texture::SharedPtr pStageTexture[2];
    } mComputeFilterPass;

    struct 
    {
        bool Enable = true;
        uint Iteration = 3u;
        EMethod Method = EMethod::PIXEL_SHADER;
        float SigmaColor = 10.0f;
        float SigmaNormal = 10.0f;
        float SigmaDepth = 10.0f;
        uint KernelSize = 3u;
        EFilterDirection Direction = EFilterDirection::BOTH;
        bool Adaptive = true;
        float AdaptiveRatio = 0.8f;
        uint AdaptiveShiftRange = 30u;
    } mContronls;


    void __createPassResouces();
    void __preparePixelStageTexture(Texture::SharedPtr vTarget);
    void __prepareComputeStageTexture(Texture::SharedPtr vTarget);
    void __executePixelPass(RenderContext* vRenderContext, const RenderData& vRenderData, Texture::SharedPtr vVariation, Texture::SharedPtr vVarOfVar);
    void __executePixelFilter(RenderContext* vRenderContext, Texture::SharedPtr vSource, Texture::SharedPtr vTarget, EFilterDirection vDirection);
    void __executeComputePass(RenderContext* vRenderContext, const RenderData& vRenderData, Texture::SharedPtr vVariation, Texture::SharedPtr vVarOfVar);
    void __loadInternalTexture(const RenderData& vRenderData, Texture::SharedPtr& voVariation, Texture::SharedPtr& voVarOfVar);
};
