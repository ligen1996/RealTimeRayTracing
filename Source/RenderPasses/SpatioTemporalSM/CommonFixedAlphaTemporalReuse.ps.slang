Texture2D gTexMotionVector;
Texture2D gTexPrev;
Texture2D gTexCur;

cbuffer PerFrameCB : register(b0)
{
    float gAlpha;
};

float4 main(float2 texC : TEXCOORD, float4 posH : SV_POSITION) : SV_TARGET0
{
    uint2 TexDim;
    uint Levels;
    gTexCur.GetDimensions(0, TexDim.x, TexDim.y, Levels);
    
    const int2 PixelPos = int2(posH.xy);
    const float2 MotionVector = gTexMotionVector[PixelPos].xy;
    const float2 PixelPrevPos = floor((texC + MotionVector) * TexDim) + 0.5;
    
    float4 Cur = gTexCur[PixelPos];
    if (PixelPrevPos.x < 0 && PixelPrevPos.y < 0 && PixelPrevPos.x >= TexDim.x && PixelPrevPos.y >= TexDim.y)
        return Cur;

    float4 Prev = gTexPrev[PixelPrevPos];
    return lerp(Prev, Cur, gAlpha);
}
