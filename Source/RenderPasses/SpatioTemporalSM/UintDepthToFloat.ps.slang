SamplerComparisonState gCompareSampler;

cbuffer PerFrameCB : register(b0)
{
    uint gIndex;
    uint2 gMapSize;
};

Texture2DArray<uint4> gShadowMapSet;

float unpackUintDepth(uint vDepth)
{
    return 1.0f - (vDepth / float(0xffffffffu));
}

struct PsOut
{
    float Depth : SV_Target0;
    float4 Debug : SV_Target1;
};

PsOut main(float2 texC : TEXCOORD)
{
    PsOut Out;
    uint CurDepthUint = gShadowMapSet.Load(int4(texC.x * gMapSize.x, texC.y * gMapSize.y, gIndex, 0)).r;
    float Depth = unpackUintDepth(CurDepthUint);
    Out.Depth = Depth;
    Out.Debug = float4(1.0, 0.5, 0.3, 1.0);
    return Out;
}
