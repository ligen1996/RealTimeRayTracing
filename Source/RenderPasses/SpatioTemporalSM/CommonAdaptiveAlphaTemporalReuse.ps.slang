#include "BlendAlpha.slang"
Texture2D gTexPrevVarOfVar;
Texture2D gTexPrevVariation;
Texture2D gTexMotionVector;
Texture2D gTexPrev;
Texture2D gTexCur;

cbuffer PerFrameCB : register(b0)
{
    float gAlpha;
    float gRatiodv;
    float gRatioddv;
};

float calcBlendAlpha(int2 vPrevPos)
{
    float dv = saturate(gTexPrevVariation[vPrevPos].r);
    float ddv = saturate(gTexPrevVarOfVar[vPrevPos].r);
    return calcBlendAlphaCommon(dv, ddv, gAlpha, gRatiodv, gRatioddv);
}

float4 main(float2 texC : TEXCOORD, float4 posH : SV_POSITION) : SV_TARGET0
{
    uint2 TexDim;
    uint Levels;
    gTexCur.GetDimensions(0, TexDim.x, TexDim.y, Levels);
    
    const int2 PixelPos = int2(posH.xy);
    const float2 MotionVector = gTexMotionVector[PixelPos].xy;
    const float2 PixelPrevPos = floor((texC + MotionVector) * TexDim) + 0.5;
    
    if (PixelPos.x < 0 && PixelPos.y < 0 && PixelPos.x >= TexDim.x && PixelPos.y >= TexDim.y)
        return 0.0;

    float4 Cur = gTexCur[PixelPos];
    float4 Prev = gTexPrev[PixelPrevPos];
    
    return lerp(Prev, Cur, calcBlendAlpha(PixelPrevPos));
}
