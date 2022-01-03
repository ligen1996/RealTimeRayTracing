#pragma once
#include "Falcor.h"
#include "FalcorExperimental.h"
#include "RenderGraph/RenderPassHelpers.h"

using namespace Falcor;
class MS_Shadow :
    public RenderPass
{
public:
    using SharedPtr = std::shared_ptr<MS_Shadow>;

    /** Create a new render pass object.
        \param[in] pRenderContext The render context.
        \param[in] dict Dictionary of serialized parameters.
        \return A new object, or an exception is thrown if creation failed.
    */
    static SharedPtr create(RenderContext* pRenderContext = nullptr, const Dictionary& dict = {}) { return SharedPtr(new MS_Shadow); }

    virtual std::string getDesc() override;
    virtual Dictionary getScriptingDictionary() override { return Dictionary(); }
    virtual RenderPassReflection reflect(const CompileData& compileData) override;
    virtual void compile(RenderContext* pContext, const CompileData& compileData) override {};
    virtual void execute(RenderContext* pRenderContext, const RenderData& renderData) override;
    virtual void renderUI(Gui::Widgets& widget) override;
    virtual bool onMouseEvent(const MouseEvent& mouseEvent) override { return false; }
    virtual bool onKeyEvent(const KeyboardEvent& keyEvent) override { return false; }
    virtual void setScene(RenderContext* pRenderContext, const Scene::SharedPtr& pScene) override;

private:
    MS_Shadow();

    Scene::SharedPtr mpScene;
    Light::SharedConstPtr mpLight;
    Camera::SharedPtr mpLightCamera;
    GraphicsProgram::SharedPtr mpProgram;
    GraphicsState::SharedPtr mpGraphicsState;
    GraphicsVars::SharedPtr mpVars;
    Fbo::SharedPtr mpFbo;
};

