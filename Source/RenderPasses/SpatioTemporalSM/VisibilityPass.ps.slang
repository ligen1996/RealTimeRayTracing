import STSMData;

#define USE_PCF

SamplerComparisonState gSTsmCompareSampler;
SamplerState gTextureSampler;

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
};

Texture2D gDepth;
Texture2DArray gShadowMapSet;

struct psOUt
{
    float4 Visibility : SV_Target0;
    float4 Debug : SV_Target1;
};


float calcShadowFactor(float3 posW,float2 posSxy)
{
    float3 ShadowMapDim;
    gShadowMapSet.GetDimensions(ShadowMapDim.x, ShadowMapDim.y, ShadowMapDim.z);
    uint NumShadowMap = uint(ShadowMapDim.z);
    
    float Result = 0.0;
    for (uint i = 0; i < NumShadowMap; ++i)
    {
        float4 shadowPos = mul(float4(posW, 1.0f), gSTsmData.allGlobalMat[i]); //transform to light space,
        shadowPos /= shadowPos.w;

        shadowPos.xy = shadowPos.xy * float2(0.5, 0.5) + float2(0.5, 0.5);

    #ifdef FALCOR_D3D
        shadowPos.y = 1 - shadowPos.y;
    #endif

        shadowPos.z -= gSTsmData.depthBias;
        shadowPos.z = saturate(shadowPos.z);

        float3 texC = float3(shadowPos.xy, i);
        float depth = shadowPos.z;

        float res = gShadowMapSet.SampleCmpLevelZero(gSTsmCompareSampler, texC, depth).r;

        Result += res;
    }
    return saturate(Result / float(NumShadowMap));
}


float calcShadowFactorPCF(float3 posW, float2 posSxy)
{
    const int2 offset[9] =
    {
        int2(0, 0), int2(-1, -1), int2(-1, 1),
                            int2(1, -1), int2(1, 1), int2(1, 0),
                            int2(0, -1), int2(0, 1), int2(-1, 0)
    };  

    float3 ShadowMapDim;
    gShadowMapSet.GetDimensions(ShadowMapDim.x, ShadowMapDim.y, ShadowMapDim.z);
    uint NumShadowMap = uint(ShadowMapDim.z);
    float2 PixelSize = 1.0f / ShadowMapDim.xy;
    
    float Result = 0.0;
    for (uint i = 0; i < NumShadowMap; ++i)
    {
        float4 shadowPos = mul(float4(posW, 1.0f), gSTsmData.allGlobalMat[i]); //transform to light space,
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
                float3 texCoord3D = float3(CurPos, i);
                shadow += gShadowMapSet.SampleCmpLevelZero(gSTsmCompareSampler, texCoord3D, depth).r;
            }
        }

        int SampleNumbers = (2 * PcfRadius + 1) * (2 * PcfRadius + 1); //todo:
        shadow /= SampleNumbers;

        Result += shadow;
    }
    
    return Result / float(NumShadowMap);
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
    //[0, 1] -> [0, 2] -> [-1, 1]
    float2 ndcXy = (texC * 2) - 1;
    float depth = gDepth[texC * gPass.screenDimension].x;
    float3 posW = loadPosition(texC, depth);

    float4 color = float4(0, 0, 0, 0);

#ifdef USE_PCF
    float V = calcShadowFactorPCF(posW, ndcXy);
#else
    float V = calcShadowFactor(posW, ndcXy);
#endif
    color = float4(V, V, V, 1);
    
    psOUt Res;
    Res.Visibility = color; 
    Res.Debug = float4(0.0);
    return Res;
}

