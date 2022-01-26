#include "Helper.h"

using namespace Helper;

void ShadowVPHelper::__camClipSpaceToWorldSpace(float3 voViewFrustum[8])
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

    glm::mat4 invViewProj = m_Camera->getInvViewProjMatrix();
    m_Center = float3(0, 0, 0);

    for (uint32_t i = 0; i < 8; i++)
    {
        float4 crd = invViewProj * float4(clipSpace[i], 1);
        voViewFrustum[i] = float3(crd) / crd.w;
        m_Center += voViewFrustum[i];
    }

    m_Center *= (1.0f / 8.0f);

    // Calculate bounding sphere radius
    m_Radius = 0;
    for (uint32_t i = 0; i < 8; i++)
    {
        float d = glm::length(m_Center - voViewFrustum[i]);
        m_Radius = std::max(d, m_Radius);
    }
}

void ShadowVPHelper::__calPointLightShadowMatrices(PointLight::SharedConstPtr vLight)
{
    const float3 LightPos = vLight->getWorldPosition();
    const float3 LightDirection = vLight->getWorldDirection();
    __calPerspectiveMatrices(LightPos, LightDirection, vLight->getOpeningAngle());
}

void ShadowVPHelper::__calDirectionalLightShadowMatrices(DirectionalLight::SharedConstPtr vLight)
{
    m_View = glm::lookAt(m_Center, m_Center + vLight->getWorldDirection(), float3(0, 1, 0));
    m_Proj = glm::ortho(-m_Radius, m_Radius, -m_Radius, m_Radius, -m_Radius, m_Radius);
}

void ShadowVPHelper::__calRectLightShadowMatrices(RectLight::SharedConstPtr vLight)
{
    const float3 LightPos = vLight->getPosByUv(m_Uv);
    const float3 LightDirection = vLight->getDirection();
    __calPerspectiveMatrices(LightPos, LightDirection, vLight->getOpeningAngle());

    float3 LPos_B = inverse(getView())*float4(0,0,0,1);
    float3 d = LightPos - LPos_B;
}

void ShadowVPHelper::__calShadowMatrices()
{
    float3 ViewFrustum[8];
    __camClipSpaceToWorldSpace(ViewFrustum);
    
    float4x4 ShadowVP;
    switch (m_Light ->getType())
    {
    case LightType::Point:
        __calPointLightShadowMatrices(std::dynamic_pointer_cast<const PointLight>(m_Light)); break;
    case LightType::Directional:
        __calDirectionalLightShadowMatrices(std::dynamic_pointer_cast<const DirectionalLight>(m_Light)); break;
    case LightType::Rect:
        __calRectLightShadowMatrices(std::dynamic_pointer_cast<const RectLight>(m_Light)); break;
    default:
        should_not_get_here();
    }
}

void ShadowVPHelper::__calPerspectiveMatrices(float3 vPos, float3 vDirection, float vFov)
{
    const float3 LookAt = vPos + vDirection;
    float3 up(0, 1, 0);
    if (abs(glm::dot(up, vDirection)) >= 0.95f)
    {
        up = float3(1, 0, 0);
    }

    m_View = glm::lookAt(vPos, LookAt, up);
    float DistFromCenter = glm::length(vPos - m_Center);
    float nearZ = std::max(0.1f, DistFromCenter - m_Radius);
    float maxZ = std::min(m_Radius * 2, DistFromCenter + m_Radius);
    m_Proj = glm::perspective(vFov, m_AspectRatio, nearZ, maxZ);
}

