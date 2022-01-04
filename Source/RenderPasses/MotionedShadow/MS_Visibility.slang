#include "Scene/VertexAttrib.slangh"

import Scene.Raster;
import Scene.Scene;
//import Scene.HitInfo;
//import Utils.Math.MathHelpers;
//import GBufferHelpers;
//import Experimental.Scene.Material.TexLODHelpers;

#define is_valid(name) (is_valid_##name != 0)

struct PSOut
{
    float4 Visibility : SV_TARGET0;
};

VSOut vsMain(VSIn vsIn)
{
    return defaultVS(vsIn);
}

[earlydepthstencil]
PSOut psMain(VSOut vsOut, uint triangleIndex : SV_PrimitiveID, float3 barycentrics : SV_Barycentrics)
{
    PSOut pO;

    pO.Visibility = float4(0, 0.8, 0, 1);
    return pO;
}
