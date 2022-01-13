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

    static std::string toString(EFilterDirection vType);

private:
    STSM_BilateralFilter();

    static Gui::DropdownList mDirectionList;

    struct  
    {
        FullScreenPass::SharedPtr pPass;
        Fbo::SharedPtr pFbo;
        Fbo::SharedPtr pStageFbo;
    } mVFilterPass;

    struct 
    {
        bool Enable = true;
        float SigmaColor = 10.0f;
        float SigmaNormal = 10.0f;
        float SigmaDepth = 10.0f;
        uint KernelSize = 15u;
        EFilterDirection Direction = EFilterDirection::BOTH;
    } mVContronls;


    void __createPassResouces();
    void __prepareStageFbo(Texture::SharedPtr vTarget);
};
