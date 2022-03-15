#include "Scene/VertexAttrib.slangh"

import Scene.Raster;
import Scene.Scene;

cbuffer PerFrameCB
{
    //uint2 gFrameDim;
};

VSOut vsMain(VSIn vsIn)
{
    return defaultVS(vsIn);
}

[earlydepthstencil]
float4 psMain(VSOut vsOut, uint triangleIndex : SV_PrimitiveID, float3 barycentrics : SV_Barycentrics) : SV_Target0
{
    float2 TexCrd = vsOut.texC;

    return float4(1, 1, 1, 1);
}
