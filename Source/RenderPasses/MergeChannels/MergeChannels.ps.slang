Texture2D gR;
Texture2D gG;
Texture2D gB;
Texture2D gA;

inline float linearToSRGB(float lin)
{
    if (lin <= 0.0031308f)
    {
        return lin * 12.92f;
    }
    else
    {
        return pow(lin, (1.0f / 2.4f)) * (1.055f) - 0.055f;
    }
}

inline float sRGBToLinear(float srgb)
{
    if (srgb <= 0.04045f)
    {
        return srgb * (1.0f / 12.92f);
    }
    else
    {
        return pow((srgb + 0.055f) * (1.0f / 1.055f), 2.4f);
    }
}

float4 main(float2 texC : TEXCOORD) : SV_Target0
{
    uint2 texDim;
    uint levels;
    gR.GetDimensions(0, texDim.x, texDim.y, levels);

    float2 pixelPos = texC * texDim;
    int2 iPos = int2(pixelPos);
    
    float r = gR[iPos].x;
    float g = gG[iPos].x;
    float b = gB[iPos].x;
    float a = gA[iPos].x;

    r = sRGBToLinear(r);

    float3 Color = float3(r, r, r);
    //Color = saturate((Color - 0.5));

    //return float4(r,r,r,1);
    return float4(Color, 1.);
}
