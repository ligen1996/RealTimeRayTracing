#include "Scene/VertexAttrib.slangh"

import Scene.Raster;
import Scene.Scene;
//import Scene.HitInfo;
//import Utils.Math.MathHelpers;
//import GBufferHelpers;
//import Experimental.Scene.Material.TexLODHelpers;

// UAV output channels
RasterizerOrderedTexture2D<uint4> gId;
//RasterizerOrderedTexture2D<float4> gId;

cbuffer PerFrameCB
{
    float4x4 gShadowMat;
};

#define is_valid(name) (is_valid_##name != 0)

struct SShadowVSOut
{
    uint ID : DRAW_ID;
    float4 PosH : SV_POSITION;
};

//struct PSOut
//{
//    float4 posW : SV_TARGET0;
//};

SShadowVSOut vsMain(VSIn vsIn)
{
    SShadowVSOut VSOut;
    const GeometryInstanceID instanceID = { vsIn.meshInstanceID };
    float4x4 WorldMat = gScene.getWorldMatrix(instanceID);
    float3 PosW = mul(float4(vsIn.pos, 1.f), WorldMat).xyz;
    //VSOut.PosH = mul(float4(PosW, 1.f), gScene.camera.getViewProj());
    VSOut.PosH = mul(float4(PosW, 1.f), gShadowMat);
    VSOut.ID = vsIn.meshInstanceID;

    return VSOut;
}

[earlydepthstencil]
void psMain(SShadowVSOut vsOut, uint triangleIndex : SV_PrimitiveID, float3 barycentrics : SV_Barycentrics) : SV_TARGET
{
    // Using vOut.posH.xy as pixel coordinate since it has the SV_Position semantic.
    int2 ipos = int2(vsOut.PosH.xy);

    float ui = (float(vsOut.ID) + 1) / 10;
    //gId[ipos] = float4(ui, 0, 0, 1);
    gId[ipos] = uint4(vsOut.ID, 0, 0, 1);
}
