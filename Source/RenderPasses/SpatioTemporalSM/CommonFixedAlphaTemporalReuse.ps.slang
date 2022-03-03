#include "Reuse.slang"
Texture2D gTexMotionVector;
Texture2D gTexPrev;
Texture2D gTexCur;

cbuffer PerFrameCB : register(b0)
{
    uint gReuseSampleType = 0u;
    float gAlpha;
    float4x4 gViewProjMatrix;
    float gDiscardByPositionStrength;
    float gDiscardByNormalStrength;
};

struct SOut
{
    float4 dv : SV_Target0;
    float4 Debug : SV_Target1;
};

SOut main(float2 texC : TEXCOORD, float4 posH : SV_POSITION) : SV_TARGET0
{
    uint2 TexDim;
    uint Levels;
    gTexCur.GetDimensions(0, TexDim.x, TexDim.y, Levels);
    
    const int2 PixelPos = int2(posH.xy);
    const float2 MotionVector = gTexMotionVector[PixelPos].xy;
    const float2 PixelPrevPos = getPrevPos(texC, MotionVector, TexDim);

    SOut Out;
    
    float4 Cur = gTexCur[PixelPos];
    float4 Prev = float4(0.0);
    bool valid = getPrev(gReuseSampleType, gTexPrev, PixelPos, PixelPrevPos, TexDim, gViewProjMatrix, gDiscardByPositionStrength, gDiscardByNormalStrength, Prev);
    float4 dv = valid ? lerp(Prev, Cur, gAlpha) : 0.0;
    Out.dv = dv;
    Out.Debug = float4(float3(valid ? 0.0 : 1.0), 1.0);
    return Out;
}
