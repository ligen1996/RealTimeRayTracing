#include "Reuse.slang"
Texture2D<float> gTexVisibility;
Texture2D gTexPrevVisibility;
Texture2D<float> gTexReliability;
Texture2D gTexMotionVector;

cbuffer PerFrameCB : register(b0)
{
    uint gReuseSampleType = 0u;
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
    const float2 PixelPrevPos = getPrevPos(texC, MotionVector, TexDim);
    
    float CurVis = gTexVisibility[PixelPos].r;
    float4 TempPrevVis = float4(0.0);
    bool valid = getPrev(gReuseSampleType, gTexPrevVisibility, PixelPos, PixelPrevPos, TexDim, gViewProjMatrix, gDiscardByPositionStrength, gDiscardByNormalStrength, TempPrevVis);
        
    float dv = 0.0;
    if (valid)
    {
        float PrevVis = TempPrevVis.r;

        dv = abs(CurVis - PrevVis);
        dv = saturate(dv + (1 - Reliability) * gReliabilityStrength);
    }
    
    //return dv;
    SOut Out;
    Out.dv = dv;
    Out.Debug = 0.0;
    return Out;
}
