import STSMData;

//#define USE_PCF

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
Texture2D gShadowMap;

struct psOUt
{
    float4 Visibility : SV_Target0;
    float4 Debug : SV_Target1;
};

float calcShadowFactor(float3 posW,float2 posSxy)
{
    float4 shadowPos = mul(float4(posW, 1.0f), gSTsmData.globalMat); //transform to light space,
    shadowPos /= shadowPos.w;

    shadowPos.xy = shadowPos.xy * float2(0.5, 0.5) + float2(0.5, 0.5);

#ifdef FALCOR_D3D
    shadowPos.y = 1 - shadowPos.y;
#endif

    shadowPos.z -= gSTsmData.depthBias;
    shadowPos.z = saturate(shadowPos.z);

    float2 texC = shadowPos.xy;
    float depth = shadowPos.z;

    float res = gShadowMap.SampleCmpLevelZero(gSTsmCompareSampler, texC, depth).r;
    return saturate(res);
}


float calcShadowFactorPCF(float3 posW, float2 posSxy)
{

    const int2 offset[9] = { int2(0, 0), int2(-1, -1), int2(-1,  1),
                            int2(1, -1), int2(1,  1), int2( 1,  0),
                            int2(0, -1), int2(0,  1), int2(-1,  0)};

    float2 ShadowMapDim;
    gShadowMap.GetDimensions(ShadowMapDim.x, ShadowMapDim.y);
    float2 PixelSize = 1.0f / ShadowMapDim;

    
    float4 shadowPos = mul(float4(posW, 1.0f), gSTsmData.globalMat); //transform to light space,
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
    
    //[unroll]
    //for (int k = 0; k < 9;++k)
    //{
    //    float2 CurPos = texC + offset[k] * PixelSize;
        
    //    shadow += gShadowMap.SampleCmpLevelZero(gSTsmCompareSampler, CurPos, depth).r;
    //}

    for (int x = -PcfRadius; x <= PcfRadius;++x)
    {
        for (int y = -PcfRadius; y <= PcfRadius;++y)
        {
            float2 CurPos = texC + int2(x, y) * PixelSize;
            shadow += gShadowMap.SampleCmpLevelZero(gSTsmCompareSampler, CurPos, depth).r;
        }
    }

    int SampleNumbers = (2 * PcfRadius + 1) * (2 * PcfRadius + 1);
    shadow /= SampleNumbers;
    
    return shadow;
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
    Res.Visibility = color; //depth
    Res.Debug = float4(0.0);
    
    return Res;
}

