import STSMData;

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
};

Texture2D gDepth;
Texture2D gShadowMap;


struct psOUt
{
    float4 Visibility : SV_Target0;
    float4 Debug : SV_Target1;
};


float4 calcShadowFactor(float3 posW,float2 posSxy)
{
    float4 shadowPos = mul(float4(posW, 1.0f), gSTsmData.globalMat); //transform to light space,
    shadowPos /= shadowPos.w;
    
#ifdef FALCOR_D3D
    shadowPos.y = 1 - shadowPos.y;
#endif

    shadowPos.z -= gSTsmData.depthBias;
    shadowPos.z = saturate(shadowPos.z);


    float2 texC = shadowPos.xy;
    float depth = shadowPos.z;
    
    float res = gShadowMap.SampleCmpLevelZero(gSTsmCompareSampler, texC, depth).r;
    //return saturate(res);

    float test = gShadowMap.SampleLevel(gTextureSampler, texC, 0).r;
    
    return float4(depth, texC, test);

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
    //color.r = calcShadowFactor(gCsmData, depth, posW, ndcXy);

    float4 V = calcShadowFactor(posW, ndcXy);
    //if (gPass.visualizeCascades)
    //{
    //    color.gba = getBlendedCascadeColor(gCsmData, depth);
    //}


    //float shadow = gShadowMap[texC * gPass.screenDimension].x;

    //color = float4(V, V, V, 1);

    color = float4(V.r);
    
    
    psOUt Res;
    Res.Visibility = color; //depth
    Res.Debug = float4(V.a);
    
    return Res;
}

