#include "MS_Visibility.h"

namespace
{
    const char kDesc[] = "Create Shadow Map with ID";

    const std::string kProgramFile = "RenderPasses/MotionedShadow/MS_Visibility.slang";
    const std::string shaderModel = "6_2";

    // Name = pass channel name; TexName = name in shader
    //const std::string kVisibilityName = "visibility";
    const ChannelList kInChannels =
    {
        { "SMs", "gShadowMapSet",  "Light Space Depth/Shadow Map", true /* optional */, ResourceFormat::D32Float},
        { "Id", "gID",  "Light Space Instance ID", true /* optional */, ResourceFormat::RGBA32Uint},
        { "LOffs", "gLightOffset",  "Area Light Postion Offset", true /* optional */, ResourceFormat::RGBA32Float},
    };
    const ChannelList kOutChannels =
    {
        { "vis", "gVisibility",  "Scene Visibility", true /* optional */, ResourceFormat::RGBA32Float},
        { "smv", "gShadowMotionVector", "Shadow Motion Vector", true/* optional */, ResourceFormat::RGBA32Float},  
        { "debug", "gLightSpaceOBJs", "Light Space Objects Visualize", true/* optional */, ResourceFormat::RGBA32Float},  
    };
}

MS_Visibility::MS_Visibility()
{
    Program::Desc desc;
    desc.addShaderLibrary(kProgramFile).vsEntry("vsMain").psEntry("psMain");
    desc.setShaderModel(shaderModel);
    mpProgram = GraphicsProgram::create(desc);

    DepthStencilState::Desc DepthStateDesc;
    DepthStateDesc.setDepthEnabled(true);
    DepthStateDesc.setDepthWriteMask(true);
    DepthStencilState::SharedPtr pDepthState = DepthStencilState::create(DepthStateDesc);

    mpGraphicsState = GraphicsState::create();
    mpGraphicsState->setProgram(mpProgram);
    mpGraphicsState->setDepthStencilState(pDepthState);
}

std::string MS_Visibility::getDesc() { return kDesc; }

RenderPassReflection MS_Visibility::reflect(const CompileData& compileData)
{
    // Define the required resources here
    RenderPassReflection reflector;

    addRenderPassInputs(reflector, kInChannels, Resource::BindFlags::ShaderResource);
    addRenderPassOutputs(reflector, kOutChannels, Resource::BindFlags::RenderTarget);

    return reflector;
}

void MS_Visibility::execute(RenderContext* pRenderContext, const RenderData& renderData)
{
    // attach and clear output textures to fbo
    for (size_t i = 0; i < kOutChannels.size(); ++i)
    {
        Texture::SharedPtr pTex = renderData[kOutChannels[i].name]->asTexture();
        mpFbo->attachColorTarget(pTex, uint32_t(i));
    }

    pRenderContext->clearFbo(mpFbo.get(), float4(0), 1.f, 0, FboAttachmentType::All);
    if (mpScene == nullptr) return; //early return
    if (!mpVars) { mpVars = GraphicsVars::create(mpProgram.get()); }    //assure vars isn't empty

    // set input texture vars
    for (const auto& channel : kInChannels)
    {
        auto pTex = renderData[channel.name]->asTexture();
        if (pTex == nullptr)
        {
            pTex = Texture::create2D(mpFbo->getWidth(), mpFbo->getHeight(), channel.format);
            //pRenderContext->clearTexture(pTex.get());
        }
        mpVars[channel.texname] = pTex;
    }

    // set shader data(need update every frame)
    __preparePassData();
    
    mpVars["PerFrameCB"][mPassDataOffset].setBlob(mPassData);

    mpGraphicsState->setFbo(mpFbo);

    mpScene->rasterize(pRenderContext, mpGraphicsState.get(), mpVars.get());
}

void MS_Visibility::renderUI(Gui::Widgets& widget)
{
}

void MS_Visibility::setScene(RenderContext* pRenderContext, const Scene::SharedPtr& pScene)
{
    assert(pScene);
    mpScene = pScene;
    mpProgram->addDefines(mpScene->getSceneDefines());
    mpVars = GraphicsVars::create(mpProgram.get());
    mPassDataOffset = mpVars->getParameterBlock("PerFrameCB")->getVariableOffset("Data");

    if (!(mpLight = mpScene->getLightByName("Main light")))
    {
        mpLight = (mpScene && mpScene->getLightCount() ? mpScene->getLight(0) : nullptr);
    }
    assert(mpLight);
}

void MS_Visibility::compile(RenderContext* pContext, const CompileData& compileData)
{
    Fbo::Desc FboDesc;
    FboDesc.setDepthStencilTarget(ResourceFormat::D32Float);
    uint2 Dim = compileData.defaultTexDims;

    mpFbo = Fbo::create2D(Dim.x, Dim.y, FboDesc);
}

void MS_Visibility::__preparePassData()
{
    Camera::SharedConstPtr pCamera = mpScene->getCamera();
    Helper::ShadowVPHelper SVPHelper(pCamera, mpLight, (float)mPassData.ScreenDim.x / (float)mPassData.ScreenDim.y);

    mPassData.CameraInvVPMat = pCamera->getInvViewProjMatrix();
    mPassData.ScreenDim = uint2(mpFbo->getWidth(), mpFbo->getHeight());
    mPassData.ShadowProj = SVPHelper.getProj();
    mPassData.ShadowVP = SVPHelper.getVP();
    mPassData.InvShadowVP = inverse(mPassData.ShadowVP);
    mPassData.InvShadowView = inverse(SVPHelper.getView());
    mPassData.PreCamVP = pCamera->getProjMatrix() * pCamera->getPrevViewMatrix();
    __prepareLightData();
}

void MS_Visibility::__prepareLightData()
{
    if (mpLight->getType() == LightType::Point)
    {
        PointLight* pPL = (PointLight*)mpLight.get();
        mPassData.LightPos = pPL->getWorldPosition();
        mPassData.LightGridSize = 1;
        mPassData.HalfLightSize = float2(0,0);
    }
    else if (mpLight->getType() == LightType::Rect)
    {
        RectLight* pPL = (RectLight*)mpLight.get();
        mPassData.LightPos = pPL->getCenter();
        mPassData.LightGridSize = 4;
        mPassData.HalfLightSize = pPL->getSize()/float2(2.);
    }
}
