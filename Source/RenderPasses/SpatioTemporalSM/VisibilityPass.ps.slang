import Utils.Sampling.SampleGenerator;
import STSMData;

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
    int PcfRadius;
    bool gRandomSelection = true;
    uint gSelectNum = 1;
    float gTime = 0.0;
};

Texture2D gDepth;
Texture2DArray gShadowMapSet;

struct psOUt
{
    float4 Visibility : SV_Target0;
    float4 Debug : SV_Target1;
};

float calcSingleShadowMap(uint vIndex, float3 posW, float2 posSxy)
{
    float4 shadowPos = mul(float4(posW, 1.0f), gSTsmData.allGlobalMat[vIndex]); //transform to light space,
    shadowPos /= shadowPos.w;

    shadowPos.xy = shadowPos.xy * float2(0.5, 0.5) + float2(0.5, 0.5);

#ifdef FALCOR_D3D
    shadowPos.y = 1 - shadowPos.y;
#endif

    shadowPos.z -= gSTsmData.depthBias;
    shadowPos.z = saturate(shadowPos.z);

    float3 texC = float3(shadowPos.xy, vIndex);
    float depth = shadowPos.z;

    //float curDepth = gShadowMapSet.Load(int4(shadowPos.x * ShadowMapDim.x, shadowPos.y * ShadowMapDim.y, vIndex, 0)).r;
    //if (depth > curDepth)
    //    return 0.0;
    //else
    //    return 1.0;
    float res = gShadowMapSet.SampleCmpLevelZero(gSTsmCompareSampler, texC, depth).r;

    return res;
}



float calcSingleShadowMapPCF(uint vIndex, float3 posW, float2 posSxy, float2 PixelSize)
{
    const int2 offset[9] =
    {
        int2(0, 0), int2(-1, -1), int2(-1, 1),
        int2(1, -1), int2(1, 1), int2(1, 0),
        int2(0, -1), int2(0, 1), int2(-1, 0)
    };
    
    float4 shadowPos = mul(float4(posW, 1.0f), gSTsmData.allGlobalMat[vIndex]); //transform to light space,
    shadowPos /= shadowPos.w;
    shadowPos.xy = shadowPos.xy * float2(0.5, 0.5) + float2(0.5, 0.5);

#ifdef FALCOR_D3D
    shadowPos.y = 1 - shadowPos.y;
#endif

    shadowPos.z -= gSTsmData.depthBias;
    shadowPos.z = saturate(shadowPos.z);

    float2 texC = shadowPos.xy;
    float depth = shadowPos.z;

    float shadow = 0.0f;
    for (int x = -PcfRadius; x <= PcfRadius; ++x)
    {
        for (int y = -PcfRadius; y <= PcfRadius; ++y)
        {
            float2 CurPos = texC + int2(x, y) * PixelSize;
            float3 texCoord3D = float3(CurPos, vIndex);
            shadow += gShadowMapSet.SampleCmpLevelZero(gSTsmCompareSampler, texCoord3D, depth).r;
        }
    }

    int SampleNumbers = (2 * PcfRadius + 1) * (2 * PcfRadius + 1);
    shadow /= SampleNumbers;

    return shadow;
}

float calcShadowFactor(float3 posW, float2 posSxy)
{
    float3 ShadowMapDim;
    gShadowMapSet.GetDimensions(ShadowMapDim.x, ShadowMapDim.y, ShadowMapDim.z);
    uint NumShadowMap = uint(ShadowMapDim.z);
    
    float Result = 0.0;
    for (uint i = 0; i < NumShadowMap; ++i)
    {
        Result += calcSingleShadowMap(i, posW, posSxy);
    }
    return saturate(Result / float(NumShadowMap));
}

float calcShadowFactorPCF(float3 posW, float2 posSxy, uint NumShadowMap, float2 PixelSize)
{
    float Result = 0.0;
    uint SelectShadowMapNum = NumShadowMap;
    if (gRandomSelection)
        SelectShadowMapNum = min(gSelectNum, NumShadowMap);

    SampleGenerator sg = SampleGenerator.create((posSxy * 0.5 + 0.5) / PixelSize + gTime, SelectShadowMapNum);
    for (uint i = 0; i < SelectShadowMapNum; ++i)
    {
        uint ShadowMapIndex = i;
        if (gRandomSelection)
            ShadowMapIndex = uint(sampleNext1D(sg) * NumShadowMap);
        Result += calcSingleShadowMapPCF(ShadowMapIndex, posW, posSxy, PixelSize);
    }
    
    return Result / float(SelectShadowMapNum);
}


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

psOUt main(float2 texC : TEXCOORD) : SV_TARGET0
{
    // init global
    float3 ShadowMapDim;
    gShadowMapSet.GetDimensions(ShadowMapDim.x, ShadowMapDim.y, ShadowMapDim.z);
    uint NumShadowMap = uint(ShadowMapDim.z);
    float2 PixelSize = 1.0f / ShadowMapDim.xy;
    
    //[0, 1] -> [0, 2] -> [-1, 1]
    float2 ndcXy = (texC * 2) - 1;
    float depth = gDepth[texC * gPass.screenDimension].x;
    float3 posW = loadPosition(texC, depth);

    float4 color = float4(0, 0, 0, 0);

#ifdef USE_PCF
    float V = calcShadowFactorPCF(posW, ndcXy, NumShadowMap, PixelSize);
#else
    float V = calcShadowFactor(posW, ndcXy);
#endif
    color = float4(V, V, V, 1);
    
    psOUt Res;
    Res.Visibility = color; 
    Res.Debug = float4(float3(calcSingleShadowMap(0, posW, ndcXy)), 1.0);
    return Res;
}

