#include "Scene/VertexAttrib.slangh"

import Scene.Raster;
import Scene.Scene;
//import Scene.HitInfo;
//import Utils.Math.MathHelpers;
//import GBufferHelpers;
//import Experimental.Scene.Material.TexLODHelpers;

#define is_valid(name) (is_valid_##name != 0)

Texture2DArray<float> gShadowMapSet;
Texture2DArray<uint> gIDSet;
Texture2D<float2> gLightOffset; // now light offset directly stores (sm_id/sm_count)
Texture2D<float4> gPosW;
Texture2D<float4> gPrePosW;

struct SPassData
{
    float4x4 CameraInvVPMat;
    float4x4 ShadowVP;
    float4x4 InvShadowVP;
    float4x4 ShadowView;
    float4x4 InvShadowView;
    float4x4 ShadowProj;
    float4x4 InvShadowProj;
    float4x4 PreCamVP;

    float3 LightPos;
    int LightGridSize;
    float2 HalfLightSize;

    uint2 ScreenDim;
} ;

struct SShadowMapData
{
    float4x4 allGlobalMat[16];
    float4x4 allInvGlobalMat[16];
    float2 allUv[16];
};

struct SLightData
{
    float4 allLightPos[16];
    float4 allLightPrePos[16];
};

cbuffer PerFrameCB
{
    SShadowMapData gShadowMapData;
    SLightData gLightData;
    SPassData Data;
};

struct PSOut
{
    float4 Visibility : SV_TARGET0;
    float4 ShadowMotionVector : SV_TARGET1;
    float4 Reliability : SV_TARGET2;
    float4 Debug : SV_TARGET3;
};

VSOut vsMain(VSIn vsIn)
{
    return defaultVS(vsIn);
}

float3 loadPosition(float2 UV, float depth, float4x4 InvVPMat)
{
    //float2 invSize = 1.0f / Data.ScreenDim;
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

float2 homoToUV(float4 vPosH)
{
    float2 LightUV = vPosH.xy;
    float2 w = float2(vPosH.w, vPosH.w);
#ifdef FALCOR_D3D
    w.y = -w.y;
#endif
    return LightUV/w*0.5+0.5;
}

int calShadowMapIndex(float2 vOffset)
{
    //int2 Idx = floor((vOffset * 0.5 + 0.5) * Data.LightGridSize);
    int2 Idx = ((vOffset * 0.5 + 0.5) * Data.LightGridSize);
    return Idx.x + Idx.y * Data.LightGridSize;
}

float4 calPosLightH(float4 vPosW, float4 vOffsetMulProj)
{
    return mul(vPosW, Data.ShadowVP) - vOffsetMulProj;
}

float4 calPosLightH(float4 vPosW, float2 vOffset)
{
    float2 OffW = vOffset * Data.HalfLightSize;
    float4 PosLH = mul(vPosW, Data.ShadowView);
    return mul(PosLH - float4(OffW * PosLH.w, 0, 0), Data.ShadowProj);
    //return mul(vPosW, Data.ShadowVP) - vOffsetMulProj;
}

float3 calBlockerPosW(float2 vLUV, float vDepth)
{
    float2 ndc = 2 * vLUV - 1;
#ifdef FALCOR_D3D
    ndc.y = -ndc.y;
#endif
    float4 wsPos = mul(float4(ndc.x, ndc.y, vDepth, 1.f), Data.InvShadowVP);
    // still problem
    return (wsPos).xyz / wsPos.w;
}

float3 calBlockerPosW(float2 vLUV, float vDepth, float4 vOffsetMulProj)
{
    float2 ndc = 2 * vLUV - 1;
#ifdef FALCOR_D3D
    ndc.y = -ndc.y;
#endif
    float4 wsPos = mul(float4(ndc.x, ndc.y, vDepth, 1.f), Data.InvShadowVP);
    // still problem
    return (wsPos + vOffsetMulProj).xyz / wsPos.w;
}

float3 calBlockerPosW(float2 vLUV, float vDepth,int vIndex)
{
    float2 ndc = 2 * vLUV - 1;
#ifdef FALCOR_D3D
    ndc.y = -ndc.y;
#endif
    float4 wsPos = mul(float4(ndc.x, ndc.y, vDepth, 1.f), gShadowMapData.allInvGlobalMat[vIndex]);
    // still problem
    return wsPos.xyz / wsPos.w;
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

[earlydepthstencil]
PSOut psMain(VSOut vsOut)
{
    PSOut pO;

    // initialize
    int2 ipos = int2(vsOut.posH.xy);
    bool outOfLightClipSpace = false;
    float2 LOffset = gLightOffset[ipos].xy;
    int SMIdx = LOffset.x * 16.;
    float3 LPosW = gLightData.allLightPos[SMIdx].xyz; // TODO: LPrePosW
    float4 PosW = float4(vsOut.posW, 1);
    
    float4 PosLH = mul(PosW, gShadowMapData.allGlobalMat[SMIdx]);
    float3 posLCS = PosLH.xyz / PosLH.w;

    if (abs(posLCS.x) > 1 || abs(posLCS.y) > 1 || posLCS.z > 1 || posLCS.z < 0)
        outOfLightClipSpace = true;

    float2 LightUV = homoToUV(PosLH);
    float3 ShadowMapDim;
    gShadowMapSet.GetDimensions(ShadowMapDim.x, ShadowMapDim.y, ShadowMapDim.z);
    const uint NumShadowMap = uint(ShadowMapDim.z);
    const uint2 MapSize = uint2(ShadowMapDim.xy);
    int2 LightTexCoord = int2(LightUV * float2(MapSize));

    // cal visibility
    float LightDepth = (PosLH.z / PosLH.w);
    float BlockerDepth = gShadowMapSet.Load(int4(LightTexCoord, SMIdx, 0));
    float visb = 0;
    if (saturate(LightDepth + 0.001f) < BlockerDepth)
    {
        visb = 1;
    }

    // back project
    uint BlockerID = gIDSet.Load(int4(LightTexCoord, SMIdx, 0));
    const GeometryInstanceID BlkGID = { BlockerID };

    //float3 BlockerPosW = calBlockerPosW(LightUV, BlockerDepth, LOffsetMulP);
    //float3 BlockerPosW = calBlockerPosW(LightUV, BlockerDepth);
    float3 BlockerPosW = calBlockerPosW(LightUV, BlockerDepth, SMIdx);
    float4 BlockerPrePosW = calBlockerPrePosW(BlockerPosW, BlkGID);

    // TODO:make sure rayDir*lightDir > 0
    // cal pre UV
    float3 LightRayDir = normalize(BlockerPrePosW.xyz - LPosW); //rd
    float3 ReceivePlaneNormal = normalize(vsOut.normalW); //pn
    float3 IntersectPosW = calRayPlaneIntersectPosW(LPosW, LightRayDir, PosW.xyz, ReceivePlaneNormal);

    float4 PreShadowPosH = mul(float4(IntersectPosW, 1.f), Data.PreCamVP);
    float2 PreUV = homoToUV(PreShadowPosH);
    
    pO.Visibility = float4(float3(visb), 1);
    float2 MotionVec = PreUV - (vsOut.posH.xy) / Data.ScreenDim;

    pO.ShadowMotionVector = float4(MotionVec, 0, 1);

    // cal reliability
    float3 Differ = gPrePosW[PreUV * Data.ScreenDim].xyz - PosW.xyz;
    float3 Normal = ReceivePlaneNormal;
    float Cos = dot(normalize(Differ), normalize(Normal));
    pO.Reliability = exp((-(Cos) * (Cos)) / (0.02));

    float visId = gIDSet.Load(int4(LightTexCoord, SMIdx, 0));

    // belowing is for DEBUG
    float flag = 0;
    //for (int i = 0; i < 16;i++)
    //{
    //    if (abs(LOffset - gShadowMapData.allUv.load))
    //}
    //pO.Debug = float4((visId + 1) / 10, 0.5, 0, 1);
    pO.Debug = float4(float2(float(SMIdx * 4) / 16, float(SMIdx%4)/16), 0, 1);
    pO.Debug = float4(float3(abs(1-Cos)), 1);
    //pO.Debug = float4(abs(distance(IntersectPosW, posW.xyz)), 0, 0, 1); // diff is large(maybe few pixels in far distance)
    
    return pO; 
}
