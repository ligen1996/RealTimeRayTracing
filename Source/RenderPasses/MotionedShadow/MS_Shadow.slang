#include "Scene/VertexAttrib.slangh"

import Scene.Raster;
//import Scene.HitInfo;
//import Utils.Math.MathHelpers;
//import GBufferHelpers;
//import Experimental.Scene.Material.TexLODHelpers;

// UAV output channels
RasterizerOrderedTexture2D<float4> gId;

cbuffer PerFrameCB
{
    
};

#define is_valid(name) (is_valid_##name != 0)

//struct PSOut
//{
//    float4 posW : SV_TARGET0;
//};

VSOut vsMain(VSIn vsIn)
{
    //VSOut vsout;
    //return vsout;
    return defaultVS(vsIn);
}

[earlydepthstencil]
void psMain(VSOut vsOut, uint triangleIndex : SV_PrimitiveID, float3 barycentrics : SV_Barycentrics)
{
    // Using vOut.posH.xy as pixel coordinate since it has the SV_Position semantic.
    int2 ipos = int2(vsOut.posH.xy);

    gId[ipos] = float4(float(vsOut.instanceID), 0, 0, 1);
}
