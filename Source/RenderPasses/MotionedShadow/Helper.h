#pragma once
#include "Falcor.h"
#include "FalcorExperimental.h"

using namespace Falcor;

namespace Helper
{
    void __createShadowMatrix( DirectionalLight* pLight, const float3& center, float radius, glm::mat4& voView, glm::mat4& voProj);
    void __createShadowMatrix( PointLight* pLight, const float3& center, float radius, float fboAspectRatio, glm::mat4& voView, glm::mat4& voProj);
    void __createShadowMatrix( RectLight* pLight, const float3& center, float radius, float fboAspectRatio, glm::mat4& voView, glm::mat4& voProj);
    void __createShadowMatrix( Light* pLight, const float3& center, float radius, float fboAspectRatio, glm::mat4& voView, glm::mat4& voProj);
    void __camClipSpaceToWorldSpace(const Camera* pCamera, float3 viewFrustum[8], float3& center, float& radius);
    float4x4 getShadowVP(Camera* vCamera,  Light* vLight, float vAspectRatio);
    void getShadowViewAndProj(Camera* vCamera,  Light* vLight, float vAspectRatio,float4x4& voView,float4x4& voProj);
    void getShadowVPAndInv(Camera* vCamera,  Light* vLight, float vAspectRatio, float4x4& voShadowVP, float4x4& voInvShadowVP);
}
