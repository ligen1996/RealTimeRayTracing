Texture2D gTexColor;
Texture2D gTexPrevColor;
Texture2D gTexMotionVector;

float luminance(float3 rgb)
{
    return 0.2126 * rgb.r + 0.7152 * rgb.g + 0.0722 * rgb.b;
}

float main(float2 texC : TEXCOORD, float4 posH : SV_POSITION) : SV_TARGET0
{
    uint2 TexDim;
    uint Levels;
    gTexColor.GetDimensions(0, TexDim.x, TexDim.y, Levels);
    
    const int2 PixelPos = int2(posH.xy);
    const float2 MotionVector = gTexMotionVector[PixelPos].xy;
    const int2 PixelPrevPos = int2(float2(PixelPos) + MotionVector.xy * TexDim + float2(0.5, 0.5));
    
    if (PixelPos.x < 0 && PixelPos.y < 0 && PixelPos.x >= TexDim.x && PixelPos.y >= TexDim.y)
        return 0.0;

    float CurLumi = luminance(gTexColor[PixelPos].rgb);
    float PrevLumi = luminance(gTexPrevColor[PixelPrevPos].rgb);
    float Mean = (CurLumi + PrevLumi) * 0.5;
    float Variance = (pow(CurLumi - Mean, 2) + pow(PrevLumi - Mean, 2)) * 0.5;
    Variance = clamp(Variance, 0, 1);
    return abs(CurLumi - PrevLumi);
    return (1 - Variance);
}
