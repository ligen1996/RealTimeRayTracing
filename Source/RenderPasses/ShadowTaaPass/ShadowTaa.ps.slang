


cbuffer PerFrameCB : register(b0)
{
    float gAlpha;
    float gBoxSigma;//use it at clamp
    //float gEnableClamp;
}

//Texture2D gTexColor; //Shading Color rgba
Texture2D gTexCurVisibility; //only r component
Texture2D gTexPreVisibility; //last frame data
Texture2D gTexVMotionVec;//rgba:rg motion vector for visibility
SamplerState gSampler; //unknown if ues it


float main(float2 texC : TEXCOORD) : SV_Target0
{
    const int2 offset[8] ={ int2(-1, -1), int2(-1, 1), int2(1, -1), int2(1, 1),
                            int2(1, 0), int2(0, -1), int2(0, 1), int2(-1, 0) };
    
    uint2 texDim;
    uint levels;
    gTexCurVisibility.GetDimensions(0, texDim.x, texDim.y, levels);

    float2 pixelPos = texC * texDim;

    int2 iPos = int2(pixelPos);

    float curV = gTexCurVisibility.Load(int3(iPos, 0)).r;
    float3 motion = gTexVMotionVec.Load(int3(iPos, 0)).rgb;

    if (motion.b == 0.0) //imply not in shadow
        return 1.0f;
    
    float VAvg = curV;
    float VVar = curV * curV;

    
    [unroll]
    for (int k = 0; k < 8; k++)
    {
        float tempV = gTexCurVisibility.Load(int3(iPos + offset[k], 0)).r;
        VAvg += tempV;
        VVar += tempV * tempV;
    }
    
    float oneOverNine = 1.0 / 9.0;
    VAvg *= oneOverNine;
    VVar *= oneOverNine;

    float sigma = sqrt(max(0.0f, VVar - VAvg * VAvg));
    float VMin = VAvg - gBoxSigma * sigma;
    float VMax = VAvg + gBoxSigma * sigma;

    const int2 iposPrev = int2(pixelPos + motion.xy * texDim + float2(0.5f, 0.5f));

    float preV = gTexPreVisibility[iposPrev].r;

    //todo: adjust alpha
    float disToClamp = min(abs(VMin - preV), abs(VMax - preV));
    float alpha = gAlpha;

    preV = clamp(preV, VMin, VMax);

    float resultV = lerp(preV, curV, alpha);

    //float3 color = gTexColor.Load(int3(iPos, 0)).rgb;

    //color *= resultV;
    
    //return float4(color, 1.0f);

    return resultV;
}
