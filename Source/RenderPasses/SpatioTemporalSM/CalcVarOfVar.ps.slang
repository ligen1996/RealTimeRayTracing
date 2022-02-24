Texture2D<float> gTexCur;
Texture2D<float> gTexPrev;
Texture2D gMotionVector;
Texture2D<float> gReliability;

cbuffer PerFrameCB : register(b0)
{
    float gReliabilityStrength = 1.0f;
}

float main(float2 texC : TEXCOORD, float4 pos : SV_POSITION) : SV_Target0
{
    uint2 TexDim;
    uint Levels;
    gTexCur.GetDimensions(0, TexDim.x, TexDim.y, Levels);
    
    const int2 PixelPos = int2(pos.xy);
    const float Reliability = gReliability[PixelPos];
    const float2 MotionVector = gMotionVector[PixelPos].xy;
    const float2 PixelPrevPos = floor((texC + MotionVector) * TexDim) + 0.5;
    
    float Cur = gTexCur[PixelPos];
    if (PixelPrevPos.x < 0 && PixelPrevPos.y < 0 && PixelPrevPos.x >= TexDim.x && PixelPrevPos.y >= TexDim.y)
        return Cur;
    
    float Prev = gTexPrev[PixelPrevPos];
    float Delta = abs(Cur - Prev);
    Delta = saturate(Delta + (1 - Reliability) * gReliabilityStrength);
    return Delta;
}
