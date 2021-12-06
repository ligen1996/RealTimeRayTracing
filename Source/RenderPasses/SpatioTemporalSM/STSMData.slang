#pragma once
#include "Utils/HostDeviceShared.slangh"

BEGIN_NAMESPACE_FALCOR


struct StsmData
{
    float4x4 globalMat;
    float depthBias = 0.005f;
    float3 lightDir;
    float farZ;

    //not use
    float2 evsmExponents = float2(5.54f, 3.0f); // posExp, negExp
    float cascadeBlendThreshold = 0.0f;
#ifndef HOST_CODE
    Texture2DArray shadowMap;
   // SamplerState csmSampler;
#endif
};

#ifdef HOST_CODE
static_assert(sizeof(StsmData) % sizeof(float4) == 0, "CsmData size should be aligned on float4 size");
#endif


END_NAMESPACE_FALCOR
