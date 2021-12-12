

cbuffer PerFrameCB : register(b0)
{
    float gAlpha;
    uint2 gRenderTargetDim;
}

Texture2D gTexVisibility;
Texture2D gTexPrevVisiblity;
//todo motion vector

float4 main(float2 texC : TEXCOORD, float4 posH : SV_POSITION) : SV_Target
{
    float2 pixelPos = posH.xy;
    int2 iPos = int2(pixelPos);
    
    float currentV = gTexVisibility[iPos].r;
    float prevoiusV = gTexPrevVisiblity[iPos].r;

    float resultV = lerp(prevoiusV, currentV, gAlpha);//todo adjust alpha
    //float resultV = currentV;

    return float4(resultV);
}
