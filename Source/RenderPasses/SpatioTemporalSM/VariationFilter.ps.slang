#include "ReuseFactorEstimationConstant.slangh"

cbuffer PerFrameCB : register(b0)
{
    uint gFilterType;
    uint gKernelSize;
    uint gDirection;
}

Texture2D<float> gTexVariation;

float filterTent(int2 vPixelPos, uint2 vScreenDim)
{
    const int HalfKernelSize = int((gKernelSize - 1) / 2);

    int2 NewPos;
    float Result = 0.0f;
    float TapWeight = 1.0f / float(gKernelSize);

	[unroll]
    for (int i = -HalfKernelSize; i <= HalfKernelSize; ++i)
    {
        if (gDirection == 0u)
            NewPos = vPixelPos + int2(i, 0);
        else
            NewPos = vPixelPos + int2(0, i);
        
        if (NewPos.x < 0 && NewPos.y < 0 && NewPos.x >= vScreenDim.x && NewPos.y >= vScreenDim.y)
            continue;

        float Variation = gTexVariation[NewPos].r;
        Result += Variation * TapWeight;
    }

    return Result;
}

float filterMax(int2 vPixelPos, uint2 vScreenDim)
{
    const int HalfKernelSize = int((gKernelSize - 1) / 2);

    int2 NewPos;
    float Result = 0.0f;

	[unroll]
    for (int i = -HalfKernelSize; i <= HalfKernelSize; ++i)
    {
        if (gDirection == 0u)
            NewPos = vPixelPos + int2(i, 0);
        else
            NewPos = vPixelPos + int2(0, i);
        
        if (NewPos.x < 0 && NewPos.y < 0 && NewPos.x >= vScreenDim.x && NewPos.y >= vScreenDim.y)
            continue;

        float Variation = gTexVariation[NewPos].r;
        
        Result = max(Result, Variation);
    }

    return Result;
}

float main(float2 uvs : TEXCOORD, float4 pos : SV_POSITION) : SV_TARGET0
{
    uint2 TexDim;
    uint Levels;
    gTexVariation.GetDimensions(0, TexDim.x, TexDim.y, Levels);
    
    if (gFilterType == _FILTER_TYPE_MAX)
    {
        return filterMax(pos.xy, TexDim);
    }
    else if (gFilterType == _FILTER_TYPE_TENT)
    {
        return filterTent(pos.xy, TexDim);
    }
    else
    {
        return 0.0;
    }
}
