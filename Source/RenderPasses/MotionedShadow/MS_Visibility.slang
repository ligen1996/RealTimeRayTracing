#include "Scene/VertexAttrib.slangh"

import Scene.Raster;
import Scene.Scene;
//import Scene.HitInfo;
//import Utils.Math.MathHelpers;
//import GBufferHelpers;
//import Experimental.Scene.Material.TexLODHelpers;

#define is_valid(name) (is_valid_##name != 0)

Texture2D gSM;
Texture2D<uint4> gID;

struct SPassData
{
    float4x4 CameraInvVPMat;
    float4x4 ShadowVP;
    float4x4 InvShadowVP;
    float3 LightPos;
    float Padding;

    uint2 ScreenDim;
} ;

cbuffer PerFrameCB
{
    SPassData Data;
};

struct PSOut
{
    float4 Visibility : SV_TARGET0;
    float4 ShadowMotionVector : SV_TARGET1;
};

VSOut vsMain(VSIn vsIn)
{
    return defaultVS(vsIn);
}

float3 loadPosition(float2 UV, float depth, float4x4 InvVPMat)
{
    float2 invSize = 1.0f / Data.ScreenDim;
    // Recompute position by unprojecting the depth stored in the z-buffer 
    float2 ndc = 2 * UV + invSize - 1;
    // Need to invert the y. For Vulkan, the clip-space is inverted, so no need to do anything
#ifdef FALCOR_D3D
    ndc.y = -ndc.y;
#endif
    float4 wsPos = mul(float4(ndc.x, ndc.y, depth, 1.f), InvVPMat);
    return wsPos.xyz / wsPos.w;
}

[earlydepthstencil]
PSOut psMain(VSOut vsOut)
{
    PSOut pO;
    int2 ipos = int2(vsOut.posH.xy);

    //float2 uv = vsOut.posH.xy / Data.ScreenDim;
    //float2 ndcXy = (uv * 2) - 1;
    //ndcXy.y = -ndcXy.y;
    //float depth = gSM[ipos].x;
    //float3 posW = loadPosition(uv, depth,Data.CameraInvVPMat);

    float4 posW = float4(vsOut.posW, 1);
    float4 posLightH = mul(posW, Data.ShadowVP);

    float2 LightTexCoord = posLightH.xy;
    LightTexCoord.y = -LightTexCoord.y;
    LightTexCoord /= posLightH.w ;
    LightTexCoord *= 0.5 ;
    LightTexCoord += 0.5 ;// norm ipos
    float2 LightUV = LightTexCoord;
    LightTexCoord *= Data.ScreenDim;//ipos
    // depth is correct
    float LightDepth = (posLightH.z / posLightH.w);
    float BlockerDepth = gSM[(int2) LightTexCoord].r;

    float visb = (BlockerDepth + 0.005 < LightDepth) ? 0 : 1;

    //uint BlockerID = gID[ios].r;
    uint BlockerID = gID[LightTexCoord].r;
    const GeometryInstanceID BlkGID = { BlockerID };

    float3 BlockerPosW = loadPosition(LightUV, BlockerDepth, Data.InvShadowVP);

    float4x4 BlkInvWM = gScene.getInverseWorldMatrix4x4(BlkGID);
    float4 BlockerPosLocal = mul(float4(BlockerPosW,1),BlkInvWM);

    float4x4 BlkPreWM = gScene.getPrevWorldMatrix(BlkGID);
    float4 BlockerPrePosW = mul(BlockerPosLocal, BlkPreWM);

    // make sure dir*lightDir > 0
    float3 LightRayDir = normalize(BlockerPrePosW.xyz - Data.LightPos);//d
    float3 ReceivePlaneNormal = vsOut.normalW;//N
    float t = dot((posW.xyz - Data.LightPos), ReceivePlaneNormal) / dot(LightRayDir, ReceivePlaneNormal);//t=(x_0-Origin)*N/d*N
    float3 IntersectPosW = Data.LightPos + t * LightRayDir;
    float4 PreShadowPosH = mul(float4(IntersectPosW, 1.f), gScene.camera.getViewProj());
    float2 PreUV = PreShadowPosH.xy;
    PreUV.y = -PreUV.y;
    PreUV /= PreShadowPosH.w;
    PreUV *= 0.5;
    PreUV += 0.5;

    //pO.Visibility = float4(float(BlockerID+1)/10., 0, 0, 1);
    pO.Visibility = float4(float3(visb), 1);
    pO.ShadowMotionVector = float4(PreUV - vsOut.posH.xy / Data.ScreenDim, 0, 1);
    return pO; 
}
