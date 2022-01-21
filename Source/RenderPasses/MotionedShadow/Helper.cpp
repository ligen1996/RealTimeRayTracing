#include "Helper.h"

void Helper::__createShadowMatrix(DirectionalLight* pLight, const float3& center, float radius, glm::mat4& voView, glm::mat4& voProj)
{
    voView = glm::lookAt(center, center + pLight->getWorldDirection(), float3(0, 1, 0));
    voProj = glm::ortho(-radius, radius, -radius, radius, -radius, radius);
}

void Helper::__createShadowMatrix(PointLight* pLight, const float3& center, float radius, float fboAspectRatio, glm::mat4& voView, glm::mat4& voProj)
{
    const float3 lightPos = pLight->getWorldPosition();
    const float3 lookat = pLight->getWorldDirection() + lightPos;
    float3 up(0, 1, 0);
    if (abs(glm::dot(up, pLight->getWorldDirection())) >= 0.95f)
    {
        up = float3(1, 0, 0);
    }

    float distFromCenter = glm::length(lightPos - center);
    float nearZ = std::max(0.1f, distFromCenter - radius);
    float maxZ = std::min(radius * 2, distFromCenter + radius);
    float angle = pLight->getOpeningAngle() * 2;

    voView = glm::lookAt(lightPos, lookat, up);
    voProj = glm::perspective(angle, fboAspectRatio, nearZ, maxZ);
}

void Helper::__createShadowMatrix(RectLight* pLight, const float3& center, float radius, float fboAspectRatio, glm::mat4& voView, glm::mat4& voProj)
{
    const float3 lightPos = pLight->getCenter();
    const float3 lookat = pLight->getDirection() + lightPos;
    float3 up(0, 1, 0);
    if (abs(glm::dot(up, pLight->getDirection())) >= 0.95f)
    {
        up = float3(1, 0, 0);
    }

    float distFromCenter = glm::length(lightPos - center);
    float nearZ = std::max(0.1f, distFromCenter - radius);
    float maxZ = std::min(radius * 2, distFromCenter + radius);
    float angle = pLight->getOpeningAngle() * 2;

    voView = glm::lookAt(lightPos, lookat, up);
    voProj = glm::perspective(angle, fboAspectRatio, nearZ, maxZ);
}

void Helper::__createShadowMatrix(Light* pLight, const float3& center, float radius, float fboAspectRatio, glm::mat4& voView, glm::mat4& voProj)
{
    switch (pLight->getType())
    {
    case LightType::Directional:
        return __createShadowMatrix((DirectionalLight*)pLight, center, radius, voView, voProj);
    case LightType::Point:
        return __createShadowMatrix((PointLight*)pLight, center, radius, fboAspectRatio, voView, voProj);
    default:
        should_not_get_here();
    }
}
void Helper::__camClipSpaceToWorldSpace(const Camera* pCamera, float3 viewFrustum[8], float3& center, float& radius)
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

float4x4 Helper::getShadowVP(Camera* vCamera, Light* vLight, float vAspectRatio)
{
    float4x4 View, Proj;
    getShadowViewAndProj(vCamera, vLight, vAspectRatio, View, Proj);
    return Proj*View;
}

void Helper::getShadowViewAndProj(Camera* vCamera, Light* vLight, float vAspectRatio, float4x4 &voView, float4x4 &voProj)
{
    struct
    {
        float3 crd[8];
        float3 center;
        float radius;
    } camFrustum;
    Helper::__camClipSpaceToWorldSpace(vCamera, camFrustum.crd, camFrustum.center, camFrustum.radius);
    Helper::__createShadowMatrix(vLight, camFrustum.center, camFrustum.radius, vAspectRatio, voView, voProj);
}

void Helper::getShadowVPAndInv(Camera* vCamera, Light* vLight, float vAspectRatio, float4x4& voShadowVP, float4x4& voInvShadowVP)
{
    voShadowVP = getShadowVP(vCamera, vLight, vAspectRatio);
    voInvShadowVP = glm::inverse(voShadowVP);
}
