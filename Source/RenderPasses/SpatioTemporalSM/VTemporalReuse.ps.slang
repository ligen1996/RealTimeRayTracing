
cbuffer PerFrameCB : register(b0)
{
    float gAlpha;
    uint2 gRenderTargetDim;
    int gSampleCount;
    int gCurIndex;
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
   
    //float resultV = lerp(prevoiusV, currentV, gAlpha);//todo adjust alpha

    float resultV = currentV + prevoiusV;

    if (gSampleCount == gCurIndex + 1)
        resultV /= float(gSampleCount);
    
    return float4(float3(resultV),1.0);
}
