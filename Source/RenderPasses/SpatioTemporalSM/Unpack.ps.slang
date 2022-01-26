SamplerComparisonState gCompareSampler;

cbuffer PerFrameCB : register(b0)
{
    uint gIndex;
    uint2 gMapSize;
};

Texture2DArray<uint4> gInfoSet;

inline void unpack(uint vInfo, out float voDepth, out uint voId)
{
    voDepth = 1.0f - ((vInfo >> 8) / float(0xffffffu));
    voId = (vInfo & 0x000000ffu);
}

struct PsOut
{
    float Depth : SV_Target0;
    uint Id : SV_Target1;
};

PsOut main(float2 texC : TEXCOORD)
{
    PsOut Out;
    uint Info = gInfoSet.Load(int4(texC.x * gMapSize.x, texC.y * gMapSize.y, gIndex, 0)).r;
    unpack(Info, Out.Depth, Out.Id);
    return Out;
}
