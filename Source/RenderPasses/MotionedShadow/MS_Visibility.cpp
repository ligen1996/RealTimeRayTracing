#include "MS_Visibility.h"

namespace
{
    const char kDesc[] = "Create Shadow Map with ID";

    const std::string kProgramFile = "RenderPasses/MotionedShadow/MS_Visibility.slang";
    const std::string shaderModel = "6_2";

    // Name = pass channel name; TexName = name in shader
    //const std::string kVisibilityName = "visibility";
    const ChannelList kChannels =
    {
        { "visibility", "gVisibility",  "Scene Visibility", true /* optional */, ResourceFormat::RGBA32Float},
    };
}

MS_Visibility::MS_Visibility()
{
    Program::Desc desc;
    desc.addShaderLibrary(kProgramFile).vsEntry("vsMain").psEntry("psMain");
    desc.setShaderModel(shaderModel);
    mpProgram = GraphicsProgram::create(desc);

    mpGraphicsState = GraphicsState::create();
    mpGraphicsState->setProgram(mpProgram);

    mpFbo = Fbo::create();
}

std::string MS_Visibility::getDesc() { return kDesc; }

RenderPassReflection MS_Visibility::reflect(const CompileData& compileData)
{
    // Define the required resources here
    RenderPassReflection reflector;

    addRenderPassOutputs(reflector, kChannels, Resource::BindFlags::RenderTarget);

    return reflector;
}

void MS_Visibility::execute(RenderContext* pRenderContext, const RenderData& renderData)
{
    for (size_t i = 0; i < kChannels.size(); ++i)
    {
        Texture::SharedPtr pTex = renderData[kChannels[i].name]->asTexture();
        mpFbo->attachColorTarget(pTex, uint32_t(i));
    }
    pRenderContext->clearFbo(mpFbo.get(), float4(0), 1.f, 0, FboAttachmentType::Color);

    if (mpScene == nullptr) return; //early return

    if (!mpVars) { mpVars = GraphicsVars::create(mpProgram.get()); }    //assure vars isn't empty

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

    /*if (!(mpLight = mpScene->getLightByName("Point light")))
    {
        mpLight = (mpScene && mpScene->getLightCount() ? mpScene->getLight(0) : nullptr);
    }*/
    //assert(mpLight);
}
