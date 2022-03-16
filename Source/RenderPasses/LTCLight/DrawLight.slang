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
}

Texture2D gLightTex;
SamplerState gSampler;

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
    texcrd.y = 1. - texcrd.y;
    float3 texColor = gLightTex.Sample(gSampler,texcrd).xyz;
    //float3 texColor = gLightTex[(texcrd) * texDim].xyz;
    return float4(texColor, 1.0);
    return float4(vsOut.texcrd, 0, 1.0);
}
