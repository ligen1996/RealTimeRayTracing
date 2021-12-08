struct VSIn
{
    float3 posW : POSITION;
    float3 color : COLOR;
};

struct VSOut
{
    float3 posW : POSW;
    float4 posH : SV_POSITION;
    float3 color : COLOR;
};

cbuffer PerFrameCB
{
    float4x4 LightWorldMat;
    float4x4 ViewProj; //lg
}

VSOut vsMain(VSIn vsIn, uint instanceID : SV_InstanceID)
{
    VSOut vsOut;

    float4 posW = mul(float4(vsIn.posW, 1.0f), LightWorldMat);
    vsOut.posW = posW.xyz;
    vsOut.posH = mul(posW, ViewProj);
    vsOut.color = vsIn.color;
    return vsOut;
}

float4 psMain(VSOut vsOut) : SV_TARGET
{
    return float4(vsOut.color, 1.0);
}
