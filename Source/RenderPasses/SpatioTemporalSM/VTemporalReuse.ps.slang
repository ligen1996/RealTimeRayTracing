#define _USE_MOTION_VECTOR
#define _ACCURACY 0.01

cbuffer PerFrameCB : register(b0)
{
    bool gEnableBlend;
    bool gEnableClamp = true;
    uint gClampSearchRadius = 1;
    float gClampExtendRange = 0.0;
    bool gEnableDiscardByPosition = true;
    bool gEnableDiscardByNormal = true;
    bool gAdaptiveAlpha = true;
    float gAlpha;
    float4x4 gViewProjMatrix;
}

Texture2D gTexMotionVector;
Texture2D gTexVisibility;
Texture2D gTexPrevVisiblity;
Texture2D gTexCurPos;
Texture2D gTexPrevPos;
Texture2D gTexCurNormal;
Texture2D gTexPrevNormal;
Texture2D gTexAlpha;

struct psOut
{
    float4 Result : SV_Target0;
    float4 Debug : SV_Target1;
};

psOut main(float2 texC : TEXCOORD, float4 posH : SV_POSITION) : SV_Target
{
    float2 pixelPos = posH.xy;
    int2 iPos = int2(pixelPos);
    uint2 texDim;
    uint levels;
    gTexVisibility.GetDimensions(0, texDim.x, texDim.y, levels);
    const float2 imageDim = float2(texDim);
    
    float currentV = gTexVisibility[iPos].r;

    float resultV = 0.0;
    float4 debug = float4(0.0);
    if (gEnableBlend)
    {
#ifdef _USE_MOTION_VECTOR
        resultV = currentV;
        
        float2 motionVector = gTexMotionVector[iPos].xy;
        const float2 iposPrev = float2(iPos) + motionVector.xy * imageDim + float2(0.5, 0.5);
        //float alpha = gAlpha;
        float Variation = gTexAlpha[iPos].r;
        float alpha = gAlpha;
        if (gAdaptiveAlpha)
            alpha *= (1 - Variation);

        if (iposPrev.x > 0 && iposPrev.y > 0 && iposPrev.x < texDim.x && iposPrev.y < texDim.y)
        {
            bool discardPrev = false;
            float prevoiusV = gTexPrevVisiblity[iposPrev].r;
            
            // clamp
            bool Clamped = false;
            if (gEnableClamp)
            {
                float minV = 1.0, maxV = 0.0;
                int r = int(gClampSearchRadius);
                for (int i = -r; i <= r; ++i)
                {
                    for (int k = -r; k <= r; ++k)
                    {
                        float nearV = gTexVisibility[iPos + float2(i, k)].r;
                        minV = min(minV, nearV);
                        maxV = max(maxV, nearV);
                    }
                }
                const float Padding = gClampExtendRange;
                if (prevoiusV < minV - Padding - _ACCURACY || prevoiusV > maxV + Padding + _ACCURACY)
                {
                    Clamped = true;
                }
                prevoiusV = clamp(prevoiusV, minV - Padding, maxV + Padding); // simple clamp
            }
        
            // check if position is acceptable
            bool discardByPosition = false;
            if (gEnableDiscardByPosition)
            {
                float3 CurPos = gTexCurPos[iPos].xyz;
                float4 CurPosH = mul(float4(CurPos, 1.0), gViewProjMatrix);
                float Depth = CurPosH.z / CurPosH.w;
                float3 PrevPos = gTexPrevPos[iposPrev].xyz;
                float MaxOffset = pow(Depth, 2) * 0.05 + 0.01; // further => larger acceptable offset
                if (distance(CurPos, PrevPos) > MaxOffset)
                {
                    discardPrev = discardByPosition = true;
                }
            }
        
            // check if normal is acceptable
            bool discardByNormal = false;
            if (gEnableDiscardByNormal)
            {
                float3 CurNormal = gTexCurNormal[iPos].xyz;
                float3 PrevNormal = gTexPrevNormal[iposPrev].xyz;
                if (distance(CurNormal, PrevNormal) > 0.1)
                {
                    discardPrev = discardByNormal = true;
                }
            }
        
            // debug output
            //debug = float4(Clamped ? 1.0 : 0.0, discardPrev ? 1.0 : 0.0, 0.0, 1.0);
            //debug = float4(Clamped ? 1.0 : 0.0, discardByPosition ? 1.0 : 0.0, discardByNormal ? 1.0 : 0.0, 1.0);
            debug = float4(float3(Variation), 1.0);

            if (!discardPrev)
                resultV = lerp(prevoiusV, currentV, alpha);
        }
#else
    float prevoiusV = gTexPrevVisiblity[iPos].r;
    resultV = lerp(prevoiusV, currentV, gAlpha);
    debug = float4(0.0);
#endif
    }
    else
    {
        resultV = currentV;
    }
    
    psOut Output;
    Output.Result = float4(resultV, resultV, resultV, 1.0);
    Output.Debug = debug;
    return Output;
}
