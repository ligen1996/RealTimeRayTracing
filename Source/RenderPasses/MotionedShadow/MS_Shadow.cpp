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
        { "Id", "gId",  "Instance Id", true /* optional */, ResourceFormat::RGBA32Float},
    };
}

std::string MS_Shadow::getDesc() { return kDesc; }

// Don't remove this. it's required for hot-reload to function properly
//extern "C" __declspec(dllexport) const char* getProjDir()
//{
//    return PROJECT_DIR;
//}

static void createShadowMatrix(const DirectionalLight* pLight, const float3& center, float radius, glm::mat4& shadowVP)
{
    glm::mat4 view = glm::lookAt(center, center + pLight->getWorldDirection(), float3(0, 1, 0));
    glm::mat4 proj = glm::ortho(-radius, radius, -radius, radius, -radius, radius);

    shadowVP = proj * view;
}
static void createShadowMatrix(const PointLight* pLight, const float3& center, float radius, float fboAspectRatio, glm::mat4& shadowVP)
{
    const float3 lightPos = pLight->getWorldPosition();
    const float3 lookat = pLight->getWorldDirection() + lightPos;
    float3 up(0, 1, 0);
    if (abs(glm::dot(up, pLight->getWorldDirection())) >= 0.95f)
    {
        up = float3(1, 0, 0);
    }

    glm::mat4 view = glm::lookAt(lightPos, lookat, up);
    float distFromCenter = glm::length(lightPos - center);
    float nearZ = std::max(0.1f, distFromCenter - radius);
    float maxZ = std::min(radius * 2, distFromCenter + radius);
    float angle = pLight->getOpeningAngle() * 2;
    glm::mat4 proj = glm::perspective(angle, fboAspectRatio, nearZ, maxZ);

    shadowVP = proj * view;
}
static void createShadowMatrix(const Light* pLight, const float3& center, float radius, float fboAspectRatio, glm::mat4& shadowVP)
{
    switch (pLight->getType())
    {
    case LightType::Directional:
        return createShadowMatrix((DirectionalLight*)pLight, center, radius, shadowVP);
    case LightType::Point:
        return createShadowMatrix((PointLight*)pLight, center, radius, fboAspectRatio, shadowVP);
    default:
        should_not_get_here();
    }
}
void camClipSpaceToWorldSpace(const Camera* pCamera, float3 viewFrustum[8], float3& center, float& radius)
{
    float3 clipSpace[8] =
    {
        float3(-1.0f, 1.0f, 0),
        float3(1.0f, 1.0f, 0),
        float3(1.0f, -1.0f, 0),
        float3(-1.0f, -1.0f, 0),
        float3(-1.0f, 1.0f, 1.0f),
        float3(1.0f, 1.0f, 1.0f),
        float3(1.0f, -1.0f, 1.0f),
        float3(-1.0f, -1.0f, 1.0f),
    };

    glm::mat4 invViewProj = pCamera->getInvViewProjMatrix();
    center = float3(0, 0, 0);

    for (uint32_t i = 0; i < 8; i++)
    {
        float4 crd = invViewProj * float4(clipSpace[i], 1);
        viewFrustum[i] = float3(crd) / crd.w;
        center += viewFrustum[i];
    }

    center *= (1.0f / 8.0f);

    // Calculate bounding sphere radius
    radius = 0;
    for (uint32_t i = 0; i < 8; i++)
    {
        float d = glm::length(center - viewFrustum[i]);
        radius = std::max(d, radius);
    }
}

MS_Shadow::MS_Shadow()
{
    Program::Desc desc;
    desc.addShaderLibrary(kProgramFile).vsEntry("vsMain").psEntry("psMain");
    desc.setShaderModel(shaderModel);
    mpProgram = GraphicsProgram::create(desc);

    mpGraphicsState = GraphicsState::create();
    mpGraphicsState->setProgram(mpProgram);

    mpFbo = Fbo::create();
}

RenderPassReflection MS_Shadow::reflect(const CompileData& compileData)
{
    // Define the required resources here
    RenderPassReflection reflector;

    reflector.addOutput(kDepthName, "Depth buffer").format(ResourceFormat::D32Float).bindFlags(Resource::BindFlags::DepthStencil);
    reflector.addOutput(kIdName, "ID buffer").format(ResourceFormat::RGBA32Float).bindFlags(ResourceBindFlags::UnorderedAccess);

    return reflector;
}

void MS_Shadow::execute(RenderContext* pRenderContext, const RenderData& renderData)
{
    // renderData holds the requested resources
    // auto& pTexture = renderData["src"]->asTexture();
    auto clear = [&](const ChannelDesc& channel)
    {
        auto pTex = renderData[channel.name]->asTexture();
        if (pTex) {
            pRenderContext->clearUAV(pTex->getUAV().get(), float4(0.f));
        }
    };
    for (const auto& channel : kChannels) clear(channel);

    if (mpScene == nullptr) return;

    if (!mpVars)
    {
        mpVars = GraphicsVars::create(mpProgram.get());
    }

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

    mpLight = (mpScene && mpScene->getLightCount() ? mpScene->getLight(0) : nullptr);
}
