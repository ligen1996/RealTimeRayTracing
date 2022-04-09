#include "ShadowMapConstant.slangh"
import Utils.Sampling.SampleGenerator;

//#define NO_REPEAT

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
    bool gUseLightTexture;
    float gLightAvg;
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

float biasDepth(float vDepth, float vScaleFactor)
{
    //vDepth -= gDepthBias;
    vDepth -= gDepthBias * vScaleFactor;
    vDepth = saturate(vDepth);
    return vDepth;
}

float calcSingleShadowMap(float3 vPosW, uint2 vMapSize, uint vIndex, float vScaleFactor)
{
    float3 uvw = getUVW(vPosW, vIndex);
    float CurDepth = biasDepth(uvw.z, vScaleFactor);
    
    if (uvw.x > 1 || uvw.x < 0 || uvw.y > 1 || uvw.y < 0 || uvw.z > 1 || uvw.z < 0)
        return 1.0;
    
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
    float MapDepth = biasDepth(uvw.z, 1.0);

    //if (uvw.x > 1 || uvw.x < 0 || uvw.y > 1 || uvw.y < 0 || uvw.z > 1 || uvw.z < 0)
    //    return 1.0;

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

float4 calcShadowFactor(float3 vPosW, float2 vPosSxy, float vScaleFactor, out float voResult, out float2 voUv)
{
    // init const
    float3 ShadowMapDim;
    gShadowMapSet.GetDimensions(ShadowMapDim.x, ShadowMapDim.y, ShadowMapDim.z);
    const uint NumShadowMap = uint(ShadowMapDim.z);
    const uint2 MapSize = uint2(ShadowMapDim.xy);
        
    uint SelectShadowMapNum = min(gSelectNum, NumShadowMap);

    SampleGenerator sg = SampleGenerator.create((vPosSxy * 0.5 + 0.5) * MapSize + gTime, SelectShadowMapNum);
    voResult = 0.0;
    voUv = float2(0.0);
#ifdef NO_REPEAT
    bool RepeatMark[_SHADOW_MAP_NUM];
    for (int i = 0; i < _SHADOW_MAP_NUM; ++i)
        RepeatMark[i] = false;
#endif
    float4 Debug = float4(0);
    float2 LightUv;
    for (uint i = 0; i < SelectShadowMapNum; ++i)
    {
        uint ShadowMapIndex = i;
        if (gRandomSelection)
        {
            ShadowMapIndex = uint(sampleNext1D(sg) * NumShadowMap);
#ifdef NO_REPEAT
            // TODO: better no repeat random method?
            while (RepeatMark[ShadowMapIndex])
                ShadowMapIndex = uint(sampleNext1D(sg) * NumShadowMap);
            RepeatMark[ShadowMapIndex] = true;
#endif
        }

        // select a random used shadow map as uv
        if (i == 0u)
        {
            if (gRandomSelection)
            {
                LightUv = gShadowMapData.allUvAndIntensity[ShadowMapIndex].xy;
            }
            else
            {
                uint RandomIndex = uint(sampleNext1D(sg) * SelectShadowMapNum); // random index in selected num
                LightUv = gShadowMapData.allUvAndIntensity[RandomIndex].xy;
            }
            voUv = float2(float(ShadowMapIndex) / 16, 0);
        }
        
        const float Intensity = gShadowMapData.allUvAndIntensity[ShadowMapIndex].z;
        float Vis = calcSingleShadowMap(vPosW, MapSize, ShadowMapIndex, vScaleFactor);
        if (gUseLightTexture && Vis > 0)
        {
            Vis *= Intensity / gLightAvg;
        }

        Debug += float4(float3(Intensity / SelectShadowMapNum), 0.0);
         
        voResult += Vis;
    }
    voResult /= float(SelectShadowMapNum);

    return Debug;
    return float4(LightUv * 0.5 + 0.5, 0.0, 1.0);
}

psOUt main(float2 texC : TEXCOORD) : SV_TARGET0
{
    //[0, 1] -> [0, 2] -> [-1, 1]
    float2 ndcXy = (texC * 2) - 1;
    float depth = gDepth[texC * gScreenDimension].x;
    float3 posW = loadPosition(texC, depth);

    //float ScaleFactor = pow(depth, 64); // trick, the further the bigger bias, bug too close
    float ScaleFactor = 1.0; // trick, the further the bigger bias

    float4 color = float4(0, 0, 0, 0);
    float V;
    float2 Uv;
    psOUt Res;
    Res.Debug = calcShadowFactor(posW, ndcXy, ScaleFactor, V, Uv);
    Res.Visibility = float4(V, V, V, 1);
    Res.LightUv = Uv;
    
    if (depth > 0.999999)
    {
        Res.Visibility = float4(1);
    }
    return Res;
}
