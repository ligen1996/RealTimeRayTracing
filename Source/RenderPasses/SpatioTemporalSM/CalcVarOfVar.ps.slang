Texture2D<float> gTexCur;
Texture2D<float> gTexPrev;
Texture2D gMotionVector;
Texture2D<float> gReliability;

float main(float2 texC : TEXCOORD, float4 pos : SV_POSITION) : SV_Target0
{
    uint2 TexDim;
    uint Levels;
    gTexCur.GetDimensions(0, TexDim.x, TexDim.y, Levels);
    
    const int2 PixelPos = int2(pos.xy);
    const float Reliability = gReliability[PixelPos];
    const float2 MotionVector = gMotionVector[PixelPos].xy;
    const int2 PixelPrevPos = int2(float2(PixelPos) + MotionVector.xy * TexDim + float2(0.5, 0.5));
    
    float V1 = gTexCur[PixelPos];
    float V2 = gTexPrev[PixelPrevPos];
    float v = abs(V1 - V2);
    v = saturate(v + (1 - Reliability));
    return v;
}
