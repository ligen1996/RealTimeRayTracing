Texture2D gTexPrev;
Texture2D gTexCur;
Texture2D gTexMotionVector;

cbuffer PerFrameCB : register(b0)
{
    float gAlpha;
};

float calcBlendAlpha(int2 vPrevPos)
{
    float VarOfVar = gTexPrev[vPrevPos].r;
    float x = clamp(VarOfVar, 0, 0.1);
    return saturate(gAlpha + x);
}

float4 main(float2 texC : TEXCOORD, float4 posH : SV_POSITION) : SV_TARGET0
{
    uint2 TexDim;
    uint Levels;
    gTexCur.GetDimensions(0, TexDim.x, TexDim.y, Levels);
    
    const int2 PixelPos = int2(posH.xy);
    const float2 MotionVector = gTexMotionVector[PixelPos].xy;
    //const int2 PixelPrevPos = int2(float2(PixelPos) + MotionVector.xy * TexDim + float2(0.5, 0.5));
    const float2 PixelPrevPos = floor((texC + MotionVector) * TexDim) + 0.5;
    
    if (PixelPos.x < 0 && PixelPos.y < 0 && PixelPos.x >= TexDim.x && PixelPos.y >= TexDim.y)
        return 0.0;

    float4 CurVis = gTexCur[PixelPos];
    float4 PrevVis = gTexPrev[PixelPrevPos];
    return lerp(PrevVis, CurVis, calcBlendAlpha(PixelPrevPos));
}
