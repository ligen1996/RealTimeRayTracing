#pragma once
#include "Falcor.h"
#include "FalcorExperimental.h"

using namespace Falcor;

namespace Helper
{
    class ShadowVPHelper
    {
    public:
        ShadowVPHelper() = delete;
        ShadowVPHelper(Camera::SharedConstPtr vCamera, Light::SharedConstPtr vLight, float vAspectRatio, float2 vUv = float2(0.0))
            :m_Camera(vCamera), m_Light(vLight), m_AspectRatio(vAspectRatio), m_Uv(vUv) {__calShadowMatrices();};

        float4x4 getView() { return m_View; }
        float4x4 getProj() { return m_Proj; }
        float4x4 getVP() { return m_Proj * m_View; }
        float4x4 getInvVP() { return inverse(getVP()); }

    private:
        void __calShadowMatrices();
        void __calPointLightShadowMatrices(PointLight::SharedConstPtr vLight);
        void __calDirectionalLightShadowMatrices(DirectionalLight::SharedConstPtr vLight);
        void __calRectLightShadowMatrices(RectLight::SharedConstPtr vLight);
        void __calPerspectiveMatrices(float3 vPos, float3 vDirection, float vFov);
        void __camClipSpaceToWorldSpace(float3 voViewFrustum[8]);

        float4x4 m_View;
        float4x4 m_Proj;
        Camera::SharedConstPtr m_Camera;
        Light::SharedConstPtr m_Light;
        float m_AspectRatio;
        float m_Radius;
        float3 m_Center;
        float2 m_Uv = float2(0.0);
    };
}
