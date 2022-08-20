#include "BlendAlpha.slang"
#include "Reuse.slang"
#include "Utils/Math/MathConstants.slangh"
#define _ACCURACY 0.01

//#define M_PI                3.14159265358979323846  // pi
//#define M_PI_2              1.57079632679489661923  // pi/2
//#define M_PI_4              0.785398163397448309616 // pi/4

cbuffer PerFrameCB : register(b0)
{
    uint gReuseSampleType = 0u;
    bool gEnableBlend;
    bool gEnableClamp = true;
    uint gClampSearchRadius = 1;
    float gClampExtendRange = 0.0;
    float gDiscardByPositionStrength = 1.0f;
    float gDiscardByNormalStrength = 1.0f;
    bool gAdaptiveAlpha = true;
    float gAlpha;
    float gBeta;
    float4x4 gViewProjMatrix;
    float gRatiodv = 0.2f;
    float gRatioddv = 1.0f;
}

Texture2D gTexMotionVector;
Texture2D gTexVisibility;
Texture2D gTexPrevVisiblity;

struct psOut
{
    float4 Result : SV_Target0;
    float4 Debug : SV_Target1;
};

float clampVisibility(int2 vPos, float vVal)
{
    float MinV = 1.0, MaxV = 0.0;
    int r = int(gClampSearchRadius);
    for (int i = -r; i <= r; ++i)
    {
        for (int k = -r; k <= r; ++k)
        {
            float NearV = gTexVisibility[vPos + float2(i, k)].r;
            MinV = min(MinV, NearV);
            MaxV = max(MaxV, NearV);
        }
    }
    const float Padding = gClampExtendRange;
    return clamp(vVal, MinV - Padding, MaxV + Padding); // simple clamp
}

float gaussian(float x, float sigma)
{
    return 0.39894 * exp(-0.5 * x * x / (sigma * sigma)) / sigma;
}

float calAlpha(float theta, float alpha)
{
    return 1.0f - gaussian(theta - M_PI_2, 0.1f) * (1.0f - alpha);
}

psOut main(float2 texC : TEXCOORD, float4 posH : SV_POSITION) : SV_Target
{
    float2 pixelPos = posH.xy;
    int2 iPos = int2(pixelPos);
    uint2 texDim;
    uint levels;
    gTexVisibility.GetDimensions(0, texDim.x, texDim.y, levels);
    const float2 imageDim = float2(texDim);
    
    float currentV = gTexVisibility[iPos].r;

    float resultV = currentV;
    float4 debug = float4(0.0);
    if (gEnableBlend)
    {
        float2 motionVector = gTexMotionVector[iPos].xy;

        float theta = gTexMotionVector[iPos].z;
        
        const float2 PosPrev = getPrevPos(texC, motionVector, texDim);
        float4 TempPrev = float4(0.0);
        bool valid = getPrev(gReuseSampleType, gTexPrevVisiblity, iPos, PosPrev, texDim, gViewProjMatrix, gDiscardByPositionStrength, gDiscardByNormalStrength, TempPrev);
        
        float DebugAdaptiveAlpha = calcBlendAlpha(PosPrev, true);
        debug = float4(float3(DebugAdaptiveAlpha), 1.0);
        
        if (valid)
        {
            float prevoiusV = TempPrev.r;
               
            float NewAlpha = calAlpha(theta, gAlpha);
            // clamp
            if (gEnableClamp)
                prevoiusV = clampVisibility(iPos, prevoiusV);
            
            resultV = lerp(prevoiusV, currentV, NewAlpha);
        }
    }
    
    psOut Output;
    Output.Result = float4(resultV, resultV, resultV, 1.0);
    Output.Debug = debug;
    return Output;
}
