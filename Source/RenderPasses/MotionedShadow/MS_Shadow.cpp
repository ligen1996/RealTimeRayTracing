#include "MS_Shadow.h"

namespace
{
    const char kDesc[] = "Create Shadow Map with ID";

    const std::string kProgramFile = "RenderPasses/MotionedShadow/MS_Shadow.slang";
    const std::string shaderModel = "6_2";

    // Name = pass channel name; TexName = name in shader
    const std::string kDepthName = "depth";
    const std::string kIdName = "Id";
    const std::string kIdTexName = "gId";
    const ChannelList kChannels =
    {
        { "Id", "gId",  "Instance Id from light space", true /* optional */, ResourceFormat::RGBA32Float},
    };
}

std::string MS_Shadow::getDesc() { return kDesc; }

MS_Shadow::MS_Shadow()
{
    Program::Desc desc;
    desc.addShaderLibrary(kProgramFile).vsEntry("vsMain").psEntry("psMain");
    desc.setShaderModel(shaderModel);
    mpProgram = GraphicsProgram::create(desc);

    mpGraphicsState = GraphicsState::create();
    mpGraphicsState->setProgram(mpProgram);

    mpFbo = Fbo::create();
    mpLightCamera = Camera::create();
}

RenderPassReflection MS_Shadow::reflect(const CompileData& compileData)
{
    // Define the required resources here
    RenderPassReflection reflector;

    reflector.addOutput(kDepthName, "Depth buffer").format(ResourceFormat::D32Float).bindFlags(Resource::BindFlags::DepthStencil);
    reflector.addOutput(kIdName, "ID buffer").format(ResourceFormat::RGBA32Uint).bindFlags(ResourceBindFlags::UnorderedAccess);
    //reflector.addOutput(kIdName, "ID buffer").format(ResourceFormat::RGBA32Uint).bindFlags(ResourceBindFlags::UnorderedAccess);

    return reflector;
}

void MS_Shadow::execute(RenderContext* pRenderContext, const RenderData& renderData)
{
    // renderData holds the requested resources
    // auto& pTexture = renderData["src"]->asTexture();
    // for depth buffer, bind and clear
    const auto& pDepth = renderData[kDepthName]->asTexture();
    mpFbo->attachDepthStencilTarget(pDepth);

    mpGraphicsState->setFbo(mpFbo);
    pRenderContext->clearDsv(pDepth->getDSV().get(), 1, 0);

    // for uav , clear and bind(does the order matter? seems not)
    auto clearFunc = [&](const ChannelDesc& channel)
    {
        auto pTex = renderData[channel.name]->asTexture();
        if (pTex) {
            pRenderContext->clearUAV(pTex->getUAV().get(), uint4(0,1,0,1));
        }
    };
    for (const auto& Channel : kChannels) clearFunc(Channel);

    if (mpScene == nullptr) return; //early return
    if (!mpVars) { mpVars = GraphicsVars::create(mpProgram.get()); }    //assure vars isn't empty

    // bind uav tex names in shader
    for (const auto& channel : kChannels)
    {
        Texture::SharedPtr pTex = renderData[channel.name]->asTexture();
        mpVars[channel.texname] = pTex;
    }

    // set Buffer Datas
    mpVars["PerFrameCB"]["gShadowMat"] = __getShadowVP();

    mpGraphicsState->setFbo(mpFbo);

    mpScene->rasterize(pRenderContext, mpGraphicsState.get(), mpVars.get());
}

void MS_Shadow::renderUI(Gui::Widgets& widget)
{
}

void MS_Shadow::setScene(RenderContext* pRenderContext, const Scene::SharedPtr& pScene)
{
    assert(pScene);
    mpScene = pScene;
    mpProgram->addDefines(mpScene->getSceneDefines());
    mpVars = GraphicsVars::create(mpProgram.get());

    if (!(mpLight = mpScene->getLightByName("Point light")))
    {
        mpLight = (mpScene && mpScene->getLightCount() ? mpScene->getLight(0) : nullptr);
    }
    assert(mpLight);
}

float4x4 MS_Shadow::__getShadowVP()
{
    Camera::SharedPtr pCamera = mpScene->getCamera();
    return Helper::getShadowVP(pCamera, mpLight, (float)mpFbo->getWidth() / (float)mpFbo->getHeight());
}
