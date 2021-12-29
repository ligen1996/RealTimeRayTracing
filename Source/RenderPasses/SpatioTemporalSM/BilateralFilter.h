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

private:
    STSM_BilateralFilter();

    struct  
    {
        FullScreenPass::SharedPtr mpPass;
        Fbo::SharedPtr mpFbo;
    } mVFilterPass;

    struct 
    {
        bool Enable = true;
        float Sigma = 10.0f;
        float BSigma = 0.1f;
        uint MSize = 15u;
    } mVContronls;

    void createPassResouces();
};
