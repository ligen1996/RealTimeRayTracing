#include "ShadowMapConstant.slangh"
import Utils.Sampling.SampleGenerator;

#define USE_PCF

SamplerComparisonState gCompareSampler;

cbuffer PerFrameCB : register(b0)
{
    SShadowMapData gShadowMapData;
    float4x4 gInvViewProj;
    uint2 gScreenDimension;
    float gDepthBias = 0.00015f;
    int gPcfRadius;
    bool gRandomSelection = true;
    uint gSelectNum = 1;
    float gTime = 0.0;
};

Texture2D gDepth;
Texture2DArray<float> gShadowMapSet;

struct psOUt
{
    float4 Visibility : SV_Target0;
    float2 LightUv : SV_Target1;
    float4 Debug : SV_Target2;
};

float3 loadPosition(float2 UV, float depth)
{
    float2 invSize = 1.0f / gScreenDimension.xy;
    // Recompute position by unprojecting the depth stored in the z-buffer 
    float2 ndc = 2 * UV + invSize - 1;
    // Need to invert the y. For Vulkan, the clip-space is inverted, so no need to do anything
#ifdef FALCOR_D3D
    ndc.y = -ndc.y;
#endif
    float4 wsPos = mul(float4(ndc.x, ndc.y, depth, 1.f), gInvViewProj);
    return wsPos.xyz / wsPos.w;
}

float3 getUVW(float3 vPosW, int vMapIndex)
{
    float4 shadowPos = mul(float4(vPosW, 1.0f), gShadowMapData.allGlobalMat[vMapIndex]); //transform to light space,
#ifdef FALCOR_D3D
    shadowPos.y = -shadowPos.y;
#endif
    float3 uvw = (shadowPos.xyz / shadowPos.w) * 0.5 + 0.5;
    return uvw;
}

float biasDepth(float vDepth)
{
    vDepth -= gDepthBias;
    vDepth = saturate(vDepth);
    return vDepth;
}

float calcSingleShadowMap(float3 vPosW, uint2 vMapSize, uint vIndex)
{
    float3 uvw = getUVW(vPosW, vIndex);
    float CurDepth = biasDepth(uvw.z);
    
    float MapDepth = gShadowMapSet.Load(int4(uvw.x * vMapSize.x, uvw.y * vMapSize.y, vIndex, 0)).r;
    if (MapDepth < CurDepth)
        return 0.0;
    else
        return 1.0;
}

float calcSingleShadowMapPCF(float3 vPosW, uint2 vMapSize, uint vIndex)
{
    const int2 offset[9] =
    {
        int2(0, 0), int2(-1, -1), int2(-1, 1),
        int2(1, -1), int2(1, 1), int2(1, 0),
        int2(0, -1), int2(0, 1), int2(-1, 0)
    };
    
    float3 uvw = getUVW(vPosW, vIndex);
    float MapDepth = biasDepth(uvw.z);

    const float2 PixelSize = 1.0f / vMapSize;

    float V = 0.0f;
    for (int x = -gPcfRadius; x <= gPcfRadius; ++x)
    {
        for (int y = -gPcfRadius; y <= gPcfRadius; ++y)
        {
            float2 uv = uvw.xy + int2(x, y) * PixelSize;
            float CurDepth = gShadowMapSet.Load(int4(uv.x * vMapSize.x, uv.y * vMapSize.y, vIndex, 0)).r;
            if (MapDepth < CurDepth)
                V += 1.0;
        }
    }

    int SampleNumbers = (2 * gPcfRadius + 1) * (2 * gPcfRadius + 1);
    V /= SampleNumbers;

    return V;
}

void calcShadowFactor(float3 vPosW, float2 vPosSxy, out float voResult, out float2 voUv)
{
    // init const
    float3 ShadowMapDim;
    gShadowMapSet.GetDimensions(ShadowMapDim.x, ShadowMapDim.y, ShadowMapDim.z);
    const uint NumShadowMap = uint(ShadowMapDim.z);
    const uint2 MapSize = uint2(ShadowMapDim.xy);
    
    uint SelectShadowMapNum = NumShadowMap;
    if (gRandomSelection)
        SelectShadowMapNum = min(gSelectNum, NumShadowMap);

    SampleGenerator sg = SampleGenerator.create((vPosSxy * 0.5 + 0.5) * MapSize + gTime, SelectShadowMapNum);
    voResult = 0.0;
    voUv = float2(0.0);
    for (uint i = 0; i < SelectShadowMapNum; ++i)
    {
        uint ShadowMapIndex = i;
        if (gRandomSelection)
            ShadowMapIndex = uint(sampleNext1D(sg) * NumShadowMap);

        // select a random used shadow map as uv
        if (i == 0u)
        {
            if (gRandomSelection)
            {
                voUv = gShadowMapData.allUv[ShadowMapIndex];
            }
            else
            {
                uint RandomIndex = uint(sampleNext1D(sg) * SelectShadowMapNum); // random index in selected num
                voUv = gShadowMapData.allUv[RandomIndex];
            }
        }
        
#ifdef USE_PCF
        voResult += calcSingleShadowMapPCF(vPosW, MapSize, ShadowMapIndex);
#else
        voResult += calcSingleShadowMap(vPosW, MapSize, ShadowMapIndex);
#endif
    }
    voResult /= float(SelectShadowMapNum);
}

psOUt main(float2 texC : TEXCOORD) : SV_TARGET0
{
    //[0, 1] -> [0, 2] -> [-1, 1]
    float2 ndcXy = (texC * 2) - 1;
    float depth = gDepth[texC * gScreenDimension].x;
    float3 posW = loadPosition(texC, depth);

    float4 color = float4(0, 0, 0, 0);
    float V;
    float2 Uv;
    calcShadowFactor(posW, ndcXy, V, Uv);

    psOUt Res;
    Res.Visibility = float4(V, V, V, 1);
    Res.LightUv = Uv;
    Res.Debug = float4(0);
    return Res;
}
