import Scene.Raster;

cbuffer PerLightCB : register(b0)
{
    float4x4 gGlobalMat;
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
    
    vOut.pos = mul(float4(vIn.pos, 1.f), gGlobalMat);
    vOut.texC = vIn.texC;
    return vOut;
}

void psMain(ShadowPassPSIn pIn /*VSOut vsOut, uint triangleIndex : SV_PrimitiveID*/)
{
    float2 depth = saturate(pIn.pos.zz);
}
