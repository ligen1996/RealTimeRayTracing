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

// A single iteration of Bob Jenkins' One-At-A-Time hashing algorithm.
uint hash(uint x)
{
    x += (x << 10u);
    x ^= (x >> 6u);
    x += (x << 3u);
    x ^= (x >> 11u);
    x += (x << 15u);
    return x;
}

// Compound versions of the hashing algorithm I whipped together.
uint hash(uint2 v)
{
    return hash(v.x ^ hash(v.y));
}
uint hash(uint3 v)
{
    return hash(v.x ^ hash(v.y) ^ hash(v.z));
}
uint hash(uint4 v)
{
    return hash(v.x ^ hash(v.y) ^ hash(v.z) ^ hash(v.w));
}

// Construct a float with half-open range [0:1] using low 23 bits.
// All zeroes yields 0.0, all ones yields the next smallest representable value below 1.0.
float floatConstruct(uint m)
{
    const uint ieeeMantissa = 0x007FFFFFu; // binary32 mantissa bitmask
    const uint ieeeOne = 0x3F800000u; // 1.0 in IEEE binary32

    m &= ieeeMantissa; // Keep only mantissa bits (fractional part)
    m |= ieeeOne; // Add fractional part to 1.0
    float f = asfloat(m); // Range [1:2]
    return f - 1.0; // Range [0:1]
}

// Pseudo-random value in half-open range [0:1].
float random(float x)
{
    return floatConstruct(hash(asuint(x)));
}
float random(float2 v)
{
    return floatConstruct(hash(asuint(v)));
}
float random(float3 v)
{
    return floatConstruct(hash(asuint(v)));
}
float random(float4 v)
{
    return floatConstruct(hash(asuint(v)));
}

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
    for (uint i = 0; i < SelectShadowMapNum; ++i)
    {
        uint ShadowMapIndex = i;
        if (gRandomSelection)
            ShadowMapIndex = uint(random(float4(posW.x, posW.z, gTime, i)) * NumShadowMap);
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

