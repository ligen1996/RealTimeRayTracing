import Scene.Raster;

struct SVsOut
{
    float4 posH : SV_POSITION;
    float3 normal : NORMAL;
};

SVsOut vsMain(VSIn vIn)
{
    VSOut DefaultOut = defaultVS(vIn);
    SVsOut Out;
    Out.posH = DefaultOut.posH;
    Out.normal = DefaultOut.normalW;
    return Out;
}

float4 psMain(SVsOut vIn) : SV_Target
{
    return float4(normalize(vIn.normal), 1.0);
}
