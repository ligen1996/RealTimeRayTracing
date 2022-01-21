#include "Helper.h"

void Helper::camClipSpaceToWorldSpace(Camera::SharedConstPtr vCamera, float3 voViewFrustum[8], float3& voCenter, float& voRadius)
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

    glm::mat4 invViewProj = vCamera->getInvViewProjMatrix();
    voCenter = float3(0, 0, 0);

    for (uint32_t i = 0; i < 8; i++)
    {
        float4 crd = invViewProj * float4(clipSpace[i], 1);
        voViewFrustum[i] = float3(crd) / crd.w;
        voCenter += voViewFrustum[i];
    }

    voCenter *= (1.0f / 8.0f);

    // Calculate bounding sphere radius
    voRadius = 0;
    for (uint32_t i = 0; i < 8; i++)
    {
        float d = glm::length(voCenter - voViewFrustum[i]);
        voRadius = std::max(d, voRadius);
    }
}

float4x4 Helper::createPerpVP(float3 vPos, float3 vDirection, float vFov, float vAspectRatio, float3 vCenter, float vRadius)
{
    const float3 LookAt = vPos + vDirection;
    float3 up(0, 1, 0);
    if (abs(glm::dot(up, vDirection)) >= 0.95f)
    {
        up = float3(1, 0, 0);
    }

    glm::mat4 View = glm::lookAt(vPos, LookAt, up);
    float DistFromCenter = glm::length(vPos - vCenter);
    float nearZ = std::max(0.1f, DistFromCenter - vRadius);
    float maxZ = std::min(vRadius * 2, DistFromCenter + vRadius);
    glm::mat4 Proj = glm::perspective(vFov, vAspectRatio, nearZ, maxZ);

    return Proj * View;
}

float4x4 Helper::createShadowMatrix(DirectionalLight::SharedConstPtr vLight, const float3& vCenter, float vRadius)
{
    glm::mat4 View = glm::lookAt(vCenter, vCenter + vLight->getWorldDirection(), float3(0, 1, 0));
    glm::mat4 Proj = glm::ortho(-vRadius, vRadius, -vRadius, vRadius, -vRadius, vRadius);

    return Proj * View;
}

float4x4 Helper::createShadowMatrix(PointLight::SharedConstPtr vLight, const float3& vCenter, float vRadius, float vApectRatio)
{
    const float3 LightPos = vLight->getWorldPosition();
    const float3 LightDirection = vLight->getWorldDirection();
    return createPerpVP(LightPos, LightDirection, vLight->getOpeningAngle() * 2, vApectRatio, vCenter, vRadius);
}

float4x4 Helper::createShadowMatrix(RectLight::SharedConstPtr vLight, const float3& vCenter, float vRadius, float vAspectRatio, const float2& vUv)
{
    const float3 LightPos = vLight->getPosByUv(vUv);
    const float3 LightDirection = vLight->getDirection();
    const float Angle = vLight->getOpeningAngle(); // TODO: how to choose?
    return createPerpVP(LightPos, LightDirection, Angle, vAspectRatio, vCenter, vRadius);
}

float4x4 Helper::getShadowVP(Camera::SharedConstPtr vCamera, Light::SharedConstPtr vLight, float vAspectRatio, float2 vUv)
{
    float3 ViewFrustum[8], Center;
    float Radius;
    Helper::camClipSpaceToWorldSpace(vCamera, ViewFrustum, Center, Radius);

    float4x4 ShadowVP;
    switch (vLight->getType())
    {
    case LightType::Directional:
        return createShadowMatrix(std::dynamic_pointer_cast<const DirectionalLight>(vLight), Center, Radius);
    case LightType::Point:
        return createShadowMatrix(std::dynamic_pointer_cast<const PointLight>(vLight), Center, Radius, vAspectRatio);
    case LightType::Rect:
        return createShadowMatrix(std::dynamic_pointer_cast<const RectLight>(vLight), Center, Radius, vAspectRatio, vUv);
    default:
        should_not_get_here();
    }
    return ShadowVP;
}

void Helper::getShadowVPAndInv(Camera::SharedConstPtr vCamera, Light::SharedConstPtr vLight, float vAspectRatio,  float4x4& voShadowVP, float4x4& voInvShadowVP)
{
    _ASSERTE(vLight->getType() != LightType::Rect);
    voShadowVP = getShadowVP(vCamera, vLight, vAspectRatio, float2(0.0));
    voInvShadowVP = glm::inverse(voShadowVP);
}
