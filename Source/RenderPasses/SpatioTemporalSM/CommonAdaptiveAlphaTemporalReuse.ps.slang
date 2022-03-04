#include "Reuse.slang"
#include "BlendAlpha.slang"
Texture2D gTexPrevVarOfVar;
Texture2D gTexPrevVariation;
Texture2D gTexMotionVector;
Texture2D gTexPrev;
Texture2D gTexCur;

cbuffer PerFrameCB : register(b0)
{
    uint gReuseSampleType = 0u;
    float gAlpha;
    float gBeta;
    float gRatiodv;
    float gRatioddv;
    float4x4 gViewProjMatrix;
    float gDiscardByPositionStrength;
    float gDiscardByNormalStrength;
};

float calcBlendAlpha(int2 vPrevPos)
{
    float dv = saturate(gTexPrevVariation[vPrevPos].r);
    float ddv = saturate(gTexPrevVarOfVar[vPrevPos].r);
    return interpAlpha(dv, ddv, gAlpha, gBeta, gRatiodv, gRatioddv);
    //return calcBlendAlphaCommon(dv, ddv, gAlpha, gRatiodv, gRatioddv);
}

float4 main(float2 texC : TEXCOORD, float4 posH : SV_POSITION) : SV_TARGET0
{
    uint2 TexDim;
    uint Levels;
    gTexCur.GetDimensions(0, TexDim.x, TexDim.y, Levels);
    
    const int2 PixelPos = int2(posH.xy);
    const float2 MotionVector = gTexMotionVector[PixelPos].xy;
    const float2 PixelPrevPos = getPrevPos(texC, MotionVector, TexDim);
    
    float4 Cur = gTexCur[PixelPos];
    float4 Prev = float4(0.0);
    bool valid = getPrev(gReuseSampleType, gTexPrev, PixelPos, PixelPrevPos, TexDim, gViewProjMatrix, gDiscardByPositionStrength, gDiscardByNormalStrength, Prev);
    float4 Res = valid ? lerp(Prev, Cur, calcBlendAlpha(PixelPrevPos)) : 0.0;
    return Res;
}
