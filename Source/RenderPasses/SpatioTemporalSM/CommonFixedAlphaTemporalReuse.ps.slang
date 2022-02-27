#include "Discard.slang"
Texture2D gTexMotionVector;
Texture2D gTexPrev;
Texture2D gTexCur;

cbuffer PerFrameCB : register(b0)
{
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
    const float2 PixelPrevPos = floor((texC + MotionVector) * TexDim) + 0.5;

    SOut Out;
    
    float4 Cur = gTexCur[PixelPos];
    float4 Prev = gTexPrev[PixelPrevPos];
    bool valid = isValid(PixelPos, PixelPrevPos, TexDim, gViewProjMatrix, gDiscardByPositionStrength, gDiscardByNormalStrength);
    //if (!valid)
    //    return Cur;
    if (valid) 
        Out.dv = lerp(Prev, Cur, gAlpha);
    else
        Out.dv = 0.0;
    Out.Debug = float4(float3(valid ? 0.0 : 1.0), 1.0);
    //return lerp(Prev, Cur, gAlpha);
    return Out;

}
