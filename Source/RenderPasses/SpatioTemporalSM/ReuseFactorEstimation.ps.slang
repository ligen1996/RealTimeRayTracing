Texture2D<float> gTexVisibility;
Texture2D<float> gTexPrevVisibility;
Texture2D<float> gTexReliability;
Texture2D gTexMotionVector;

cbuffer PerFrameCB : register(b0)
{
    float gAlpha = 1.0f;
    float gReliabilityStrength = 1.0f;
}

float main(float2 texC : TEXCOORD, float4 posH : SV_POSITION) : SV_TARGET0
{
    uint2 TexDim;
    uint Levels;
    gTexVisibility.GetDimensions(0, TexDim.x, TexDim.y, Levels);
    
    const int2 PixelPos = int2(posH.xy);
    const float Reliability = gTexReliability[PixelPos];
    const float2 MotionVector = gTexMotionVector[PixelPos].xy;
    const int2 PixelPrevPos = int2(float2(PixelPos) + MotionVector.xy * TexDim + float2(0.5, 0.5));
    
    float CurVis = gTexVisibility[PixelPos].r;
    
    if (PixelPrevPos.x < 0 && PixelPrevPos.y < 0 && PixelPrevPos.x >= TexDim.x && PixelPrevPos.y >= TexDim.y)
        return CurVis;

    float PrevVis = gTexPrevVisibility[PixelPrevPos].r;
    float dv = abs(CurVis - PrevVis);
    dv = saturate(dv + (1 - Reliability) * gReliabilityStrength);
    
    return dv;
}
