#include "ShadowMapConstant.slangh"
#include "STSMData.slang"
import Utils.Sampling.SampleGenerator;

#define USE_PCF

SamplerComparisonState gSTsmCompareSampler;

struct VisibilityPassData
{
    bool visualizeCascades;
    float4x4 invViewProj;
    uint2 screenDimension;
    uint mapBitsPerChannel;
};

cbuffer PerFrameCB : register(b0)
{
    StsmData gSTsmData;
    VisibilityPassData gPass;
    SShadowMapData gShadowMapData;
    int PcfRadius;
    bool gRandomSelection = true;
    uint gSelectNum = 1;
    float gTime = 0.0;
    float4x4 gTest;
};

Texture2D gDepth;
Texture2DArray<uint4> gShadowMapSet;

struct psOUt
{
    float4 Visibility : SV_Target0;
    float4 Debug : SV_Target1;
};

float3 loadPosition(float2 UV, float depth)
{
    float2 invSize = 1.0f / gPass.screenDimension.xy;
    // Recompute position by unprojecting the depth stored in the z-buffer 
    float2 ndc = 2 * UV + invSize - 1;
    // Need to invert the y. For Vulkan, the clip-space is inverted, so no need to do anything
#ifdef FALCOR_D3D
    ndc.y = -ndc.y;
#endif
    float4 wsPos = mul(float4(ndc.x, ndc.y, depth, 1.f), gPass.invViewProj);
    return wsPos.xyz / wsPos.w;
}

float unpackUintDepth(uint vDepth)
{
    return 1.0f - (vDepth / float(0xffffffffu));
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
    vDepth -= gSTsmData.depthBias;
    vDepth = saturate(vDepth);
    return vDepth;
}

float calcSingleShadowMap(float3 vPosW, uint2 vMapSize, uint vIndex)
{
    float3 uvw = getUVW(vPosW, vIndex);
    float MapDepth = biasDepth(uvw.z);
    
    uint CurDepthUint = gShadowMapSet.Load(int4(uvw.x * vMapSize.x, uvw.y * vMapSize.y, vIndex, 0)).r;
    float CurDepth = unpackUintDepth(CurDepthUint);
    if (MapDepth > CurDepth)
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
    for (int x = -PcfRadius; x <= PcfRadius; ++x)
    {
        for (int y = -PcfRadius; y <= PcfRadius; ++y)
        {
            float2 uv = uvw.xy + int2(x, y) * PixelSize;
            uint CurDepthUint = gShadowMapSet.Load(int4(uv.x * vMapSize.x, uv.y * vMapSize.y, vIndex, 0)).r;
            float CurDepth = unpackUintDepth(CurDepthUint);
            if (MapDepth < CurDepth)
                V += 1.0;
        }
    }

    int SampleNumbers = (2 * PcfRadius + 1) * (2 * PcfRadius + 1);
    V /= SampleNumbers;

    return V;
}

float calcShadowFactor(float3 vPosW, float2 vPosSxy)
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
    float Result = 0.0;
    for (uint i = 0; i < SelectShadowMapNum; ++i)
    {
        uint ShadowMapIndex = i;
        if (gRandomSelection)
            ShadowMapIndex = uint(sampleNext1D(sg) * NumShadowMap);
#ifdef USE_PCF
        Result += calcSingleShadowMapPCF(vPosW, MapSize, ShadowMapIndex);
#else
        Result += calcSingleShadowMap(vPosW, MapSize, ShadowMapIndex);
#endif
    }
    return Result / float(SelectShadowMapNum);
}

psOUt main(float2 texC : TEXCOORD) : SV_TARGET0
{
    //[0, 1] -> [0, 2] -> [-1, 1]
    float2 ndcXy = (texC * 2) - 1;
    float depth = gDepth[texC * gPass.screenDimension].x;
    float3 posW = loadPosition(texC, depth);

    float4 color = float4(0, 0, 0, 0);
    float V = calcShadowFactor(posW, ndcXy);
    color = float4(V, V, V, 1);

    psOUt Res;
    Res.Visibility = color; 
    Res.Debug = float4(0.0);
    return Res;
}
