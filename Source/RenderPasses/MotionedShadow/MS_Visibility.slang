#include "Scene/VertexAttrib.slangh"

import Scene.Raster;
import Scene.Scene;
//import Scene.HitInfo;
//import Utils.Math.MathHelpers;
//import GBufferHelpers;
//import Experimental.Scene.Material.TexLODHelpers;

#define is_valid(name) (is_valid_##name != 0)

Texture2D<float> gSM;
Texture2D<uint4> gID;

struct SPassData
{
    float4x4 CameraInvVPMat;
    float4x4 ShadowView;
    float4x4 ShadowProj;
    float4x4 InvShadowVP;
    float4x4 PreCamVP;
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
    float4 Debug : SV_TARGET2;
};

VSOut vsMain(VSIn vsIn)
{
    return defaultVS(vsIn);
}

float3 loadPosition(float2 UV, float depth, float4x4 InvVPMat)
{
    float2 invSize = 1.0f / Data.ScreenDim;
    // Recompute position by unprojecting the depth stored in the z-buffer 
    //float2 ndc = 2 * UV + invSize - 1;
    float2 ndc = 2 * UV - 1;
    // Need to invert the y. For Vulkan, the clip-space is inverted, so no need to do anything
#ifdef FALCOR_D3D
    ndc.y = -ndc.y;
#endif
    float4 wsPos = mul(float4(ndc.x, ndc.y, depth, 1.f), InvVPMat);
    return wsPos.xyz / wsPos.w;
}

float rayPlane(float3 ro,float3 rd, float4 p)
{
    return -(dot(ro, p.xyz) + p.w) / dot(rd, p.xyz);
}

float2 homoToUV(float4 vPosH)
{
    float2 LightUV = vPosH.xy;
    float2 w = float2(vPosH.w, -vPosH.w);
    return LightUV/w*0.5+0.5;
}

float4 calBlockerPrePosW(float3 vBlockerPosW, GeometryInstanceID vBlockerID)
{
    return mul(mul(float4(vBlockerPosW, 1), gScene.getInverseWorldMatrix4x4(vBlockerID)), gScene.getPrevWorldMatrix(vBlockerID));
}

float3 calRayPlaneIntersectPosW(in float3 ro,in float3 rd,in float3 po, in float3 pn)
{
    float3 dif = po - ro;
    float d1 = dot(dif, pn);
    float d2 = dot(rd, pn);
    float t = d1 / d2; //t=(x_0-Origin)*N/d*N
    return ro + t * rd;
}

//[earlydepthstencil]
PSOut psMain(VSOut vsOut)
{
    PSOut pO;
    int2 ipos = int2(vsOut.posH.xy);
    bool outOfLightClipSpace = false;

    float4 PosW = float4(vsOut.posW, 1);
    float4x4 ShadowVP = mul(Data.ShadowView, Data.ShadowProj);
    float4x4 InvShadowVP = Data.InvShadowVP;

    float4 posLightH = mul(PosW, ShadowVP);
    float3 posLCS = posLightH.xyz / posLightH.w;

    if (abs(posLCS.x) > 1 || abs(posLCS.y) > 1 || posLCS.z > 1 || posLCS.z < 0)
        outOfLightClipSpace = true;

    float2 LightUV = homoToUV(posLightH);
    float2 LightTexCoord = LightUV*Data.ScreenDim; //ipos

    // cal visibility
    float LightDepth = (posLightH.z / posLightH.w);
    float BlockerDepth = gSM[(int2) LightTexCoord];
    float visb = !(BlockerDepth + 0.005 < LightDepth) || outOfLightClipSpace ? 1 : 0;

    // back project
    uint BlockerID = gID[LightTexCoord].r;
    const GeometryInstanceID BlkGID = { BlockerID };

    float3 BlockerPosW = loadPosition(LightUV, BlockerDepth, InvShadowVP);
    float4 BlockerPrePosW = calBlockerPrePosW(BlockerPosW, BlkGID);

    // TODO:make sure rayDir*lightDir > 0
    // cal pre UV
    float3 LightRayDir = normalize(BlockerPrePosW.xyz - Data.LightPos); //rd
    float3 ReceivePlaneNormal = normalize(vsOut.normalW); //pn
    float3 IntersectPosW = calRayPlaneIntersectPosW(Data.LightPos, LightRayDir, PosW.xyz, ReceivePlaneNormal);

    float4 PreShadowPosH = mul(float4(IntersectPosW, 1.f), Data.PreCamVP);
    float2 PreUV = homoToUV(PreShadowPosH);
    
    pO.Visibility = float4(float3(visb), 1);
    float2 MotionVec = PreUV - (vsOut.posH.xy) / Data.ScreenDim;

    pO.ShadowMotionVector = float4(MotionVec, 0, 1);

    float visId = gID[ipos].r;

    pO.Debug = float4((visId + 1) / 10, 0.5, 0, 1);
    //pO.Debug = float4(abs(distance(IntersectPosW, posW.xyz)), 0, 0, 1); // diff is large(maybe few pixels in far distance)
    
    return pO; 
}

/*
    //pO.Visibility = float4(float(BlockerID+1)/10., 0, 0, 1);
    //pO.Visibility = float4(float2((vsOut.posH.xy) / Data.ScreenDim), 0, 1);
    //pO.Visibility = float4(BlockerPosW, 1);
    //pO.Visibility = float4(float2((vsOut.posH.xy) / Data.ScreenDim), 0, 1);
    //pO.Visibility = float4(IntersectPosW, 1);
    
    //pO.Debug = float4((visId + 1) / 10,0.5,0,1);
    //pO.Debug = float4(abs(BlockerPrePosW.xyz - BlockerPosW), 1); //diff = 0
    //pO.Debug = float4(abs(IntersectPosW - posW.xyz), 1); // diff is large(maybe few pixels in far distance)
    //pO.Debug = float4(abs(t-distance(posW.xyz,Data.LightPos)),0,0, 1);// the problem is on t
    //pO.Debug = float4(abs(dot(IntersectPosW-posW.xyz, ReceivePlaneNormal))*100, 0, 0, 1); // they are orthogonal?
    //pO.Debug = float4(abs(dot(normalize(Data.LightPos - posW.xyz), normalize(Data.LightPos - BlockerPosW))) - (1./255), 0, 0, 1); // res = 1(in one line)
    //pO.Debug = float4(abs(dot(normalize(Data.LightPos - posW.xyz), normalize(BlockerPosW - posW.xyz))), 0, 0, 1); // res is weird(it's about alias)
    //pO.Debug = float4(distance(BlockerPosW ,posW.xyz), 0, 0, 1); // res is weird(it's about alias)
    //pO.ShadowMotionVector = float4(BlockerPrePosW.xyz, 1);
*/
