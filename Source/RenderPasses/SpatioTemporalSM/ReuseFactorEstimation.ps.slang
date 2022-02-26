#include "Discard.slang"
Texture2D<float> gTexVisibility;
Texture2D<float> gTexPrevVisibility;
Texture2D<float> gTexReliability;
Texture2D gTexMotionVector;

cbuffer PerFrameCB : register(b0)
{
    float gAlpha = 1.0f;
    float gReliabilityStrength = 1.0f;
    float gDiscardByPositionStrength = 1.0f;
    float gDiscardByNormalStrength = 1.0f;
    float4x4 gViewProjMatrix;
}

struct SOut
{
    float dv : SV_Target0;
    float4 Debug : SV_Target1;
};

//float main(float2 texC : TEXCOORD, float4 posH : SV_POSITION) : SV_TARGET0
SOut main(float2 texC : TEXCOORD, float4 posH : SV_POSITION) : SV_TARGET0
{
    uint2 TexDim;
    uint Levels;
    gTexVisibility.GetDimensions(0, TexDim.x, TexDim.y, Levels);
    
    const int2 PixelPos = int2(posH.xy);
    const float Reliability = gTexReliability[PixelPos];
    const float2 MotionVector = gTexMotionVector[PixelPos].xy;
    const float2 PixelPrevPos = floor((texC + MotionVector) * TexDim) + 0.5;
    //const int2 PixelPrevPos = int2(float2(PixelPos) + MotionVector.xy * TexDim + float2(0.5, 0.5));
    
    float CurVis = gTexVisibility[PixelPos].r;
    
    bool valid = isValid(PixelPos, PixelPrevPos, TexDim, gViewProjMatrix, gDiscardByPositionStrength, gDiscardByNormalStrength);
    //if (!valid)
    //    return CurVis;

    float PrevVis = gTexPrevVisibility[PixelPrevPos].r;
    float dv = abs(CurVis - PrevVis);
    dv = saturate(dv + (1 - Reliability) * gReliabilityStrength);
    
    //return dv;
    SOut Out;
    Out.dv = valid ? dv : 0.0;
    Out.Debug = float4(float3(valid ? 0.0 : 1.0), 1.0);
    return Out;
}
