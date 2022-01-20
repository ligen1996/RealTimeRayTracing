#pragma once
#include "Falcor.h"
#include "FalcorExperimental.h"

using namespace Falcor;

namespace Helper
{
    void camClipSpaceToWorldSpace(Camera::SharedConstPtr vCamera, float3 voViewFrustum[8], float3& voCenter, float& voRadius);
    float4x4 createPerpVP(float3 vPos, float3 vDirection, float vFov, float vAspectRatio, float3 vCenter, float vRadius);
    float4x4 createShadowMatrix(DirectionalLight::SharedConstPtr vLight, const float3& vCenter, float vRadius);
    float4x4 createShadowMatrix(PointLight::SharedConstPtr vLight, const float3& vCenter, float vRadius, float vApectRatio);
    float4x4 createShadowMatrix(RectLight::SharedConstPtr vLight, const float3& vCenter, float vRadius, float vFboAspectRatio, const float2& vUv);

    // TODO: uv param is only for rect light, should refactor later
    float4x4 getShadowVP(Camera::SharedConstPtr vCamera, Light::SharedConstPtr vLight, float vAspectRatio, float2 vUv = float2(0.0));
    void getShadowVPAndInv(Camera::SharedConstPtr vCamera, Light::SharedConstPtr vLight, float vAspectRatio, float4x4& voShadowVP, float4x4& voInvShadowVP);
}
