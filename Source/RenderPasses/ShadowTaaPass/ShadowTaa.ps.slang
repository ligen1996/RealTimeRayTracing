
#define V_MV_TEST //test motion vector is right
//#define USE_CLAMP

cbuffer PerFrameCB : register(b0)
{
    float gAlpha;
    float gBoxSigma;//use it at clamp
    //float gEnableClamp;
    float gEnableBlending;
}

//Texture2D gTexColor; //Shading Color rgba
Texture2D gTexCurVisibility; //only r component
Texture2D gTexPreVisibility; //last frame data
Texture2D gTexVMotionVec;//rgba:rg motion vector for visibility
Texture2D gTexPreVisibilityDEBUG;

SamplerState gSampler; //unknown if ues it

struct psOut
{
    float4 FilteredV : SV_Target0;
    float4 Debug : SV_Target1;
    float4 DebugPRE : SV_Target2;    
};

psOut main(float2 texC : TEXCOORD, float4 posH : SV_POSITION)
{
    psOut Result;
    
    const int2 offset[8] ={ int2(-1, -1), int2(-1, 1), int2(1, -1), int2(1, 1),
                            int2(1, 0), int2(0, -1), int2(0, 1), int2(-1, 0) };
    
    uint2 texDim;
    uint levels;
    gTexCurVisibility.GetDimensions(0, texDim.x, texDim.y, levels);

    float2 pixelPos = posH.xy;
    int2 iPos = int2(pixelPos);

    float curV = gTexCurVisibility.Load(int3(iPos, 0)).r;
    float3 motion = gTexVMotionVec.Load(int3(iPos, 0)).rgb;

    //if (motion.b < 0.5) //imply not in shadow
    //{
    //    Result.FilteredV = float4(1);
    //    Result.Debug = float4(1);
    //    return Result;
    //}
        
    
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

    //const int2 iposPrev = int2(pixelPos + motion.xy * float2(texDim) + float2(0.5f, 0.5f));  //in this way can lead to edge blur according to pixel offset
    const int2 iposPrev = int2(float2(iPos) + motion.xy * float2(texDim) + float2(0.5f, 0.5f));

    
    float preV = gTexPreVisibility[iposPrev].r;

    //todo: adjust alpha
    float disToClamp = min(abs(VMin - preV), abs(VMax - preV));
    float alpha = gAlpha;

#ifdef USE_CLAMP
    preV = clamp(preV, VMin, VMax);
#endif
    float resultV = lerp(preV, curV, alpha); //temporal blending

    //float3 color = gTexColor.Load(int3(iPos, 0)).rgb;
    //color *= resultV;
    //return float4(color, 1.0f);

    
#ifdef V_MV_TEST
    float res = 0;
    if (curV == preV) res = 1.0; //测试用motion 获取历史帧的正确性
    float TestPreVdata = gTexPreVisibility[iposPrev].r; //NO CLAMP HISTORY DATA
    float TestPreVUseCurPixel = gTexPreVisibility[iPos].r;//HISTORY DATA USE CUR POS TO GET
    float TestCurData = gTexCurVisibility[iPos].r;
    psOut test;

    if(TestCurData > 0.0) //not in shadow,
    {
        TestPreVdata = 1.0;
        TestPreVUseCurPixel = 1.0;
    }
    
    if (gEnableBlending>0.5) //enabe Test motion vector
    {
        //test.FilteredV = float4(preV); //CLAMP DATA
        test.FilteredV = float4(TestPreVdata); // no CLAMP DATA
    }
    else
    {
        test.FilteredV = float4(TestCurData);
    }
      
    test.Debug = float4(TestPreVdata);//use motion vector to get history coord to get history data
    test.DebugPRE = float4(TestPreVUseCurPixel); //copy history tex to visualize
    return test;
#else
    float TestPreV = gTexPreVisibility[iPos].r;
    Result.FilteredV = float4(resultV);
    Result.Debug = float4(TestPreV); //cur history
    Result.DebugPRE = float4(0); 
#endif  
}
