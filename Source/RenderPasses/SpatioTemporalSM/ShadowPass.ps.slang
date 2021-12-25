import Scene.Raster;
import STSMData;
//#define _APPLY_PROJECTION

layout(binding = 0) SamplerState alphaSampler : register(s0);
layout(binding = 1) texture2D alphaMap : register(t0);

layout(binding = 2) cbuffer AlphaMapCB : register(b1)
{
    float alphaThreshold;
};
layout(binding = 3) cbuffer PerFrameCB
{
    float gTest;
};

layout(binding = 4) cbuffer PerLightCB : register(b0)
{
    StsmData gSTsmData;
}

struct ShadowPassVSOut
{
    float2 texC : TEXCOORD;
#ifdef _APPLY_PROJECTION
    float4 pos : SV_POSITION;
#else
    float4 pos : POSITION;
#endif
};

struct ShadowPassPSIn
{
    float2 texC : TexCoord;
    float4 pos : SV_POSITION;
};

ShadowPassVSOut vsMain(VSIn vIn)
{
    ShadowPassVSOut vOut;
    const GeometryInstanceID instanceID = { vIn.meshInstanceID };

    float4x4 worldMat = gScene.getWorldMatrix(instanceID);
    vOut.pos = mul(float4(vIn.pos, 1.f), worldMat);
    
#ifdef _APPLY_PROJECTION
    //vOut.pos = mul(vOut.pos, gScene.camera.getViewProj());
    vOut.pos = mul(vOut.pos, gSTsmData.globalMat);
  
#endif

    
    //float3 ndc = vOut.pos.xyz / vOut.pos.w;
    //vOut.pos.xyz = ndc;
    
    vOut.texC = vIn.texC;
    return vOut;
}



void psMain(ShadowPassPSIn pIn /*VSOut vsOut, uint triangleIndex : SV_PrimitiveID*/)
{

#ifdef TEST_ALPHA
    float alpha = alphaMap.Sample(alphaSampler, pIn.texC)._ALPHA_CHANNEL;
    if(alpha < alphaThreshold)
    {
        discard;
    }
#endif
    float2 depth = saturate(pIn.pos.zz);
    //return float4(0, 1, 0, 1);
}
