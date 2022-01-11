#pragma once
#include "Falcor.h"
#include "FalcorExperimental.h"

using namespace Falcor;

namespace Helper
{
    void createShadowMatrix(const DirectionalLight* pLight, const float3& center, float radius, glm::mat4& shadowVP);
    void createShadowMatrix(const PointLight* pLight, const float3& center, float radius, float fboAspectRatio, glm::mat4& shadowVP);
    void createShadowMatrix(const Light* pLight, const float3& center, float radius, float fboAspectRatio, glm::mat4& shadowVP);
    void camClipSpaceToWorldSpace(const Camera* pCamera, float3 viewFrustum[8], float3& center, float& radius);
    float4x4 getShadowVP(Camera* vCamera, const Light* vLight);
}
