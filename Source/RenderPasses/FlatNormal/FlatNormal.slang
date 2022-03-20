import Scene.Raster;

VSOut vsMain(VSIn vIn)
{
    return defaultVS(vIn);
}

float4 psMain(VSOut vIn) : SV_Target
{
    return float4(normalize(vIn.normalW.xyz), 1.0);
}
