#pragma once
#include "Falcor.h"
#include "FalcorExperimental.h"
#include "ShadowMapSelectorDefines.h"

using namespace Falcor;

class STSM_ShadowMapSelector : public RenderPass
{
public:
    using SharedPtr = std::shared_ptr<STSM_ShadowMapSelector>;

    static SharedPtr create(RenderContext* pRenderContext = nullptr, const Dictionary& dict = {});
    virtual std::string getDesc() override;
    virtual Dictionary getScriptingDictionary() override;
    virtual RenderPassReflection reflect(const CompileData& compileData) override;
    virtual void compile(RenderContext* pContext, const CompileData& compileData) override;
    virtual void execute(RenderContext* pRenderContext, const RenderData& renderData) override;
    virtual void setScene(RenderContext* pRenderContext, const Scene::SharedPtr& pScene) override;
    virtual void renderUI(Gui::Widgets& widget) override;
    virtual bool onMouseEvent(const MouseEvent& mouseEvent) override { return false; }
    virtual bool onKeyEvent(const KeyboardEvent& keyEvent) override { return false; }

private:
    STSM_ShadowMapSelector();

    Scene::SharedPtr mpScene;
    uint mSceneTriangleNum = 0;

    static Gui::DropdownList mGenerationTypeList;

    struct 
    {
        EShadowMapGenerationType GenerationType = EShadowMapGenerationType::RASTERIZE;
        bool EnableAutoSelection = true;
        uint TriangleNumThreshold = 30000u;
    } mVContronls;

    static std::string __toString(EShadowMapGenerationType vType);
    void __updateGenerationType();
    void __copyTextureArray(RenderContext* vRenderContext, std::shared_ptr<Texture> vSrc, std::shared_ptr<Texture> vDst);
};
