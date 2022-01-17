#pragma once
#include "Utils/HostDeviceShared.slangh"

BEGIN_NAMESPACE_FALCOR

struct StsmData
{
    float depthBias = 0.00015f;//0.005f 
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

END_NAMESPACE_FALCOR
