#include "ReuseFactorEstimationConstant.slangh"

cbuffer PerFrameCB : register(b0)
{
    uint gFilterType;
    uint gKernelSize;
    uint gDirection;
}

Texture2D<float> gTexValue;

float filterTent(int2 vPixelPos, uint2 vScreenDim)
{
    const int HalfKernelSize = int((gKernelSize - 1) / 2);

    int2 NewPos;
    float Result = 0.0f;
    float TapWeight = 1.0f / float(gKernelSize);

    int2 Offset = int2(1, 0);
    if (gDirection != 0u)
        Offset = int2(0, 1);

	[unroll]
    for (int i = -HalfKernelSize; i <= HalfKernelSize; ++i)
    {
        NewPos = vPixelPos + Offset * i;
        
        if (NewPos.x < 0 && NewPos.y < 0 && NewPos.x >= vScreenDim.x && NewPos.y >= vScreenDim.y)
            continue;

        float V = gTexValue[NewPos].r;
        Result += V * TapWeight;
    }

    return Result;
}

float filterMaxMin(int2 vPixelPos, uint2 vScreenDim)
{
    const int HalfKernelSize = int((gKernelSize - 1) / 2);

    int2 NewPos;
    float Result = 0.0f;
    if (gFilterType == _FILTER_TYPE_MIN)
        Result = 1.0;
    
    int2 Offset = int2(1, 0);
    if (gDirection != 0u)
        Offset = int2(0, 1);
    
	[unroll]
    for (int i = -HalfKernelSize; i <= HalfKernelSize; ++i)
    {
        NewPos = vPixelPos + Offset * i;
        
        if (NewPos.x < 0 && NewPos.y < 0 && NewPos.x >= vScreenDim.x && NewPos.y >= vScreenDim.y)
            continue;

        float V = gTexValue[NewPos].r;
        if (gFilterType == _FILTER_TYPE_MAX)
            Result = max(Result, V);
        else
            Result = min(Result, V);
    }

    return Result;
}

float main(float2 uvs : TEXCOORD, float4 pos : SV_POSITION) : SV_TARGET0
{
    uint2 TexDim;
    uint Levels;
    gTexValue.GetDimensions(0, TexDim.x, TexDim.y, Levels);
    
    if (gFilterType == _FILTER_TYPE_MAX || gFilterType == _FILTER_TYPE_MIN)
    {
        return filterMaxMin(pos.xy, TexDim);
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
