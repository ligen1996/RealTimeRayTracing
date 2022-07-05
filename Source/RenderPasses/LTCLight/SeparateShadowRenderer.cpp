#include "SeparateShadowRenderer.h"
#include "Utils/Sampling/SampleGenerator.h"


namespace
{
    const char kDesc[] = "Separate Shadow Renderer";

    const std::string kShaderProgramFile = "RenderPasses/LTCLight/StochasticShading.slang";
    const std::string ShaderModel = "6_2";
    const std::string kDebugProgramFile = "RenderPasses/LTCLight/DrawLight.slang";

    const std::string kLTCTex1File = "../Data/Texture/ltc_1.dds";
    const std::string kLTCTex2File = "../Data/Texture/ltc_2.dds";
    const std::string kDepth = "Depth";

    const std::string kBlueNoiseFile = "../Data/Texture/bluenoise/128/LDR_RGBA_0.png";
    //input channels
    const ChannelList kInChannels =
    {
        {"PosW",           "gPosW",            "World position",              false, ResourceFormat::RGBA32Float},
        {"NormalW",        "gNormalW",         "World normal",                false, ResourceFormat::RGBA32Float},
        {"Roughness",      "gRoughness",       "Roughness",                   false, ResourceFormat::RGBA8Unorm},
        {"DiffuseOpacigy", "gDiffuseOpacity",  "Diffuse color and opacity",   false, ResourceFormat::RGBA32Float},
        {"SpecRough",      "gSpecRough",       "Specular color and roughness",false, ResourceFormat::RGBA32Float},
    };

    //output channels
    const std::string  kColor = "Color";
    const std::string kColorDesc = "LTC Color";

}



float halton(uint32_t index, uint32_t base)
{
    // Reversing digit order in the given base in floating point.
    float result = 0.0f;
    float factor = 1.0f;

    for (; index > 0; index /= base)
    {
        factor /= base;
        result += factor * (index % base);
    }

    return result;
}

CSeparateShadow::SharedPtr CSeparateShadow::create(RenderContext* pRenderContext, const Dictionary& dict)
{
    SharedPtr pPass = SharedPtr(new CSeparateShadow);
    return pPass;
}

std::string CSeparateShadow::getDesc()
{
    return kDesc;
}

Dictionary CSeparateShadow::getScriptingDictionary()
{
    return Dictionary();
}

RenderPassReflection CSeparateShadow::reflect(const CompileData& compileData)
{
    // Define the required resources here
    RenderPassReflection reflector;
    addRenderPassInputs(reflector, kInChannels);
    reflector.addOutput(kColor, kColorDesc).format(ResourceFormat::Unknown).texture2D(0, 0, 0);
    reflector.addInput(kDepth, "Depth for draw light").format(ResourceFormat::D32Float);
    return reflector;
}

void CSeparateShadow::execute(RenderContext* pRenderContext, const RenderData& renderData)
{
    if (!mpScene) return;
    //attach texture to fbo
    const auto& pColorTex = renderData[kColor]->asTexture();
    mpFbo->attachColorTarget(pColorTex, 0);

    for (const auto& channel : kInChannels)
    {
        const auto& pTex = renderData[channel.name]->asTexture();
        mpPass[channel.texname] = pTex;
    }

    __updateRectLightProperties();

    mpPass["gLTC_1"] = m_LTC1;
    mpPass["gLTC_2"] = m_LTC2;
    mpPass["gNoiseTex"] = mBlueNoise;

    ///*mpPass["PerFrameCB"]["LightVertices"] = mPerframeCB.LightVertices;*/   //this line have errors!!!!
    //mpPass["PerFrameCB"]["LightVertices"].setBlob(mPerframeCB.LightVertices);

    auto pCamera = mpScene->getCamera();
    mPerframeCB.CamPosW = float4(pCamera->getPosition(), 1.0);
    mPerframeCB.Padding = 0.66f;
    mpPass["gSampler"] = mpSampler;
    mpPass["PerFrameCB"]["gData"].setBlob(mPerframeCB);
    mpPass["PerFrameCB"]["gLightData"].setBlob(mLightData);
    mpPass["PerFrameCB"]["gLightMat"].setBlob(mLightMatrix);

    //mpPass["gLightMat"].setBlob(mLightMatrix);
    //halton
    static int h = 0;
    if (false)//different Noise Every Frame
    {
        h = (h + 1) % 128;
    }
    else
    {
        h = 0;
    }

    float a = halton(h, 2);
    float4 A = float4(1.0, 2.0, 3.0, 4.0);
    float4 HaltonVector = float4(halton(h, 2u), halton(h, 3u), halton(h, 5u), halton(h, 7u));
    mpPass["PerFrameCB"]["Halton"] = HaltonVector;

    mpPass->execute(pRenderContext, mpFbo);

    //auto pDepth = renderData[kDepth]->asTexture();
    //mpFbo->attachDepthStencilTarget(pDepth);
    __drawDebugLight(pRenderContext);
}

void CSeparateShadow::renderUI(Gui::Widgets& widget)
{

}

void CSeparateShadow::setScene(RenderContext* pRenderContext, const Scene::SharedPtr& pScene)
{
    assert(pScene);
    mpScene = pScene;

    if (!(mpLight = std::dynamic_pointer_cast<RectLight>(mpScene->getLightByName("RectLight"))))
    {
        mpLight = std::dynamic_pointer_cast<RectLight>(mpScene && mpScene->getLightCount() ? mpScene->getLight(0) : nullptr);
    }
    assert(mpLight);
}

CSeparateShadow::CSeparateShadow()
{
    mpPass = FullScreenPass::create(kShaderProgramFile);
    mpFbo = Fbo::create();

    m_LTC1 = Texture::createFromFile(kLTCTex1File, false, false);
    m_LTC2 = Texture::createFromFile(kLTCTex2File, false, false);

    mBlueNoise = Texture::createFromFile(kBlueNoiseFile, false, false);

    Sampler::Desc Desc;
    Desc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Point);
    Desc.setAddressingMode(Sampler::AddressMode::Clamp, Sampler::AddressMode::Clamp, Sampler::AddressMode::Clamp);
    mpSampler = Sampler::create(Desc);

    __createDebugDrawerResouces();
}

void CSeparateShadow::__createDebugDrawerResouces()
{
    mpLightDebugDrawer = TriangleDebugDrawer::create();
    mpLightDebugDrawer->clear();
    mpLightDebugDrawer->setColor(float3(1));

    //progam and vars
    Program::Desc desc;
    desc.addShaderLibrary(kDebugProgramFile).vsEntry("vsMain").psEntry("psMain");
    desc.setShaderModel(ShaderModel);
    mDebugDrawerResouce.mpProgram = GraphicsProgram::create(desc);
    mDebugDrawerResouce.mpProgram->addDefine("USE_TEXTURE_LIGHT");
    mDebugDrawerResouce.mpVars = GraphicsVars::create(mDebugDrawerResouce.mpProgram.get());

    //state
    DepthStencilState::Desc DepthStateDesc;
    DepthStateDesc.setDepthEnabled(true);
    DepthStateDesc.setDepthWriteMask(true);
    DepthStateDesc.setDepthFunc(DepthStencilState::Func::Less);
    DepthStencilState::SharedPtr pDepthState = DepthStencilState::create(DepthStateDesc);

    RasterizerState::Desc wireframeDesc;
    wireframeDesc.setCullMode(RasterizerState::CullMode::None);
    mDebugDrawerResouce.mpRasterState = RasterizerState::create(wireframeDesc);

    mDebugDrawerResouce.mpGraphicsState = GraphicsState::create();
    mDebugDrawerResouce.mpGraphicsState->setProgram(mDebugDrawerResouce.mpProgram);
    mDebugDrawerResouce.mpGraphicsState->setRasterizerState(mDebugDrawerResouce.mpRasterState);
    mDebugDrawerResouce.mpGraphicsState->setDepthStencilState(pDepthState);

    //Light Quad presentation
    DebugDrawer::Quad quad;   //first L/R = Left/Right,second L/U = Lower/Upper
    quad[0] = float3(-1, -1, 0);//LL
    quad[1] = float3(-1, 1, 0);//LU
    quad[2] = float3(1, 1, 0);//RU
    quad[3] = float3(1, -1, 0);//RL


    mpLightDebugDrawer->addPoint(quad[3], float2(1, 0));
    mpLightDebugDrawer->addPoint(quad[1], float2(0, 1));
    mpLightDebugDrawer->addPoint(quad[0], float2(0, 0));
    mpLightDebugDrawer->addPoint(quad[2], float2(1, 1));
    mpLightDebugDrawer->addPoint(quad[1], float2(0, 1));
    mpLightDebugDrawer->addPoint(quad[3], float2(1, 0));

}

void CSeparateShadow::__updateRectLightProperties()
{
    assert(mpLight);
    mPerframeCB.LightRadiance = mpLight->getIntensity();

    mPerframeCB.LightVertices[0].rgb = mpLight->getPosByUv(float2(-1, -1));
    mPerframeCB.LightVertices[1].rgb = mpLight->getPosByUv(float2(1, -1));
    mPerframeCB.LightVertices[2].rgb = mpLight->getPosByUv(float2(1, 1));
    mPerframeCB.LightVertices[3].rgb = mpLight->getPosByUv(float2(-1, 1));


    mLightData.LightAxis = float4(mpLight->getDirection(),1.0f);
    mLightData.LightCenter = float4(mpLight->getCenter(), 1.0f);
    mLightData.LightExtent = mpLight->getSize();

    mLightMatrix = mpLight->getData().transMat;
}

void CSeparateShadow::__drawDebugLight(RenderContext* pRenderContext)
{
    mDebugDrawerResouce.mpGraphicsState->setFbo(mpFbo);
    const auto pCamera = mpScene->getCamera();

    //update shader resource
    mDebugDrawerResouce.mpVars["PerFrameCB"]["gMatLightLocal2PosW"] = mpLight->getData().transMat * glm::scale(glm::mat4(), float3(mpLight->getSize() * float2(0.5), 1));
    mDebugDrawerResouce.mpVars["PerFrameCB"]["gMatCamVP"] = pCamera->getViewProjMatrix();
    mDebugDrawerResouce.mpVars["PerFrameCB"]["gLightColor"] = float3(1.0, 1.0, 1.0);
    mDebugDrawerResouce.mpVars["gLightTex"] = Texture::createFromFile("../Data/Texture/1.png", false, false);//todo
    mDebugDrawerResouce.mpVars["gSampler"] = mpSampler;
    mpLightDebugDrawer->render(pRenderContext, mDebugDrawerResouce.mpGraphicsState.get(), mDebugDrawerResouce.mpVars.get(), pCamera.get());
}
