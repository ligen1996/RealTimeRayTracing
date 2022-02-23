Texture2D gTexPrevVarOfVar;
Texture2D gTexCurVarOfVar;
Texture2D gTexPrevVariation;
Texture2D gTexMotionVector;

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
    return lerp(gAlpha, 1., saturate(ddv * gRatioddv - dv * gRatiodv));
}

float4 main(float2 texC : TEXCOORD, float4 posH : SV_POSITION) : SV_TARGET0
{
    uint2 TexDim;
    uint Levels;
    gTexCurVarOfVar.GetDimensions(0, TexDim.x, TexDim.y, Levels);
    
    const int2 PixelPos = int2(posH.xy);
    const float2 MotionVector = gTexMotionVector[PixelPos].xy;
    //const int2 PixelPrevPos = int2(float2(PixelPos) + MotionVector.xy * TexDim + float2(0.5, 0.5));
    const float2 PixelPrevPos = floor((texC + MotionVector) * TexDim) + 0.5;
    
    if (PixelPos.x < 0 && PixelPos.y < 0 && PixelPos.x >= TexDim.x && PixelPos.y >= TexDim.y)
        return 0.0;

    float4 CurVis = gTexCurVarOfVar[PixelPos];
    float4 PrevVis = gTexPrevVarOfVar[PixelPrevPos];
    return lerp(PrevVis, CurVis, calcBlendAlpha(PixelPrevPos));
}
