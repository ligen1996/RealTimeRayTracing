#pragma once

struct VSIn
{
    float3 posW : POSITION;
    float3 color : COLOR;
    float2 texcrd : TEXCOORD;
};

struct VSOut
{
    float3 posW : POSW;
    float4 posH : SV_POSITION;
    float3 color : COLOR;
    float2 texcrd : TEXCOORD;
};

cbuffer PerFrameCB
{
    float4x4 gMatLightLocal2PosW;
    float4x4 gMatCamVP;
    float4 gLightColor;
}

Texture2D gLightTex;
SamplerState gSampler;

#define Pi 3.14159265358979

float2 correct(float2 uv)
{
    float minX = -cos(Pi / 10.0f);
    float maxX = -minX;
    float minY = -cos(Pi / 5.0f);
    float maxY = 1;
    return float2((2 * uv.x - (maxX + minX)) / (maxX - minX), (2 * uv.y - (maxY + minY)) / (maxY - minY));
}

float2 mapUv(float2 uv)
{
    float r = atan2(uv.y, uv.x) + Pi;
    r = r + Pi * 2.0f * (3.0f / 20.0f); // shift to start
    r = fmod(r, Pi * 2.0f * 0.2f); // mod to[0, 2 / 5pi]
    r = r - Pi * 2.0f * 0.1f; // shift to[-1 / 5pi, 1 / 5pi]
    r = abs(r); // to[0, 1 / 5pi]
    float alpha = Pi * 0.1f; // half of peek angle of star
    float starLen = sin(alpha) / sin(Pi - alpha - r); //  <= a / sinα = b / sinβ

    uv = normalize(uv);
    //uv = correct(uv * starLen);
    return uv * starLen;
}

VSOut vsMain(VSIn vsIn, uint instanceID : SV_InstanceID)
{
    VSOut vsOut;

    float4 posW = mul(float4(vsIn.posW, 1.0f), gMatLightLocal2PosW);
    vsOut.posW = posW.xyz;
    vsOut.posH = mul(posW, gMatCamVP);
    vsOut.color = vsIn.color;
    vsOut.texcrd = vsIn.texcrd;
    return vsOut;
}

float4 psMain(VSOut vsOut) : SV_TARGET
{
    //uint2 texDim;
    //uint levels;
    //gLightTex.GetDimensions(0, texDim.x, texDim.y, levels);
    float2 texcrd = vsOut.texcrd;

    // discard to shape
    float2 lightUv = texcrd * 2 - 1;
    float2 edgeUv = lightUv / max(abs(lightUv.x), abs(lightUv.y));
    float2 starUv = mapUv(edgeUv);
    if (length(lightUv) > length(starUv))
        clip(-1);
    
    texcrd.y = 1. - texcrd.y;
    float3 Color = gLightColor.xyz;
#ifdef USE_TEXTURE_LIGHT
    Color *= gLightTex.Sample(gSampler,texcrd).xyz;
#endif
    //float3 texColor = gLightTex[(texcrd) * texDim].xyz;
    return float4(Color, 1.0);
}
