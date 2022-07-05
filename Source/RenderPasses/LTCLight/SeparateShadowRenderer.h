#pragma once
#include "Falcor.h"
#include "FalcorExperimental.h"
#include "RenderGraph/RenderPassHelpers.h"
#include "sData.slangh"

using namespace Falcor;

class CSeparateShadow : public RenderPass
{
public:
    using SharedPtr = std::shared_ptr < CSeparateShadow>;

    /** Create a new render pass object.
    \param[in] pRenderContext The render context.
    \param[in] dict Dictionary of serialized parameters.
    \return A new object, or an exception is thrown if creation failed.
*/
    static SharedPtr create(RenderContext* pRenderContext = nullptr, const Dictionary& dict = {});

    virtual std::string getDesc() override;
    virtual Dictionary getScriptingDictionary() override;
    virtual RenderPassReflection reflect(const CompileData& compileData) override;
    virtual void compile(RenderContext* pContext, const CompileData& compileData) override {}
    virtual void execute(RenderContext* pRenderContext, const RenderData& renderData) override;
    virtual void renderUI(Gui::Widgets& widget) override;
    virtual bool onMouseEvent(const MouseEvent& mouseEvent) override { return false; }
    virtual bool onKeyEvent(const KeyboardEvent& keyEvent) override { return false; }
    virtual void setScene(RenderContext* pRenderContext, const Scene::SharedPtr& pScene) override;

private:
    CSeparateShadow();

    void __createDebugDrawerResouces();
    void __updateRectLightProperties();
    void __drawDebugLight(RenderContext* pRenderContext);
    //pass resource
    FullScreenPass::SharedPtr mpPass;
    Fbo::SharedPtr mpFbo;

    SData mPerframeCB;

    //scene resource
    Scene::SharedPtr mpScene;
    RectLight::SharedPtr mpLight;

    struct
    {
        float4 LightCenter;
        float4 LightAxis;
        float2 LightExtent;
    }mLightData;

    float4x4 mLightMatrix;


    //ltc data
    Texture::SharedPtr m_LTC1, m_LTC2;

    Sampler::SharedPtr mpSampler;

    //Stochastic Shading
    Texture::SharedPtr mBlueNoise;



    //Debug Drawer
    TriangleDebugDrawer::SharedPtr mpLightDebugDrawer;

    struct  
    {
        RasterPass::SharedPtr mpRaster = nullptr;
        RasterizerState::SharedPtr mpRasterState = nullptr;

        GraphicsState::SharedPtr mpGraphicsState = nullptr;
        GraphicsVars::SharedPtr mpVars = nullptr;
        GraphicsProgram::SharedPtr mpProgram = nullptr;

        //Sampler::SharedPtr mpSampler;

        float4x4 MatLightLocal2World;
        float4x4 MatCamVP;

    }mDebugDrawerResouce;

    //TODO env map
};
