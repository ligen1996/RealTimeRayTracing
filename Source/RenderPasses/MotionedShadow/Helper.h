#pragma once
#include "Falcor.h"
#include "FalcorExperimental.h"

using namespace Falcor;

namespace Helper
{
    void __createShadowMatrix(const DirectionalLight* pLight, const float3& center, float radius, glm::mat4& voView, glm::mat4& voProj);
    void __createShadowMatrix(const PointLight* pLight, const float3& center, float radius, float fboAspectRatio, glm::mat4& voView, glm::mat4& voProj);
    void __createShadowMatrix(const Light* pLight, const float3& center, float radius, float fboAspectRatio, glm::mat4& voView, glm::mat4& voProj);
    void __camClipSpaceToWorldSpace(const Camera* pCamera, float3 viewFrustum[8], float3& center, float& radius);
    float4x4 getShadowVP(Camera* vCamera, const Light* vLight, float vAspectRatio);
    void getShadowVP(Camera* vCamera, const Light* vLight, float vAspectRatio,float4x4& voView,float4x4& voProj);
    void getShadowVPAndInv(Camera* vCamera, const Light* vLight, float vAspectRatio, float4x4& voShadowVP, float4x4& voInvShadowVP);
}
