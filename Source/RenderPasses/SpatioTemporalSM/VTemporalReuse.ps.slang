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
    bool gReverseVariation = false;
    float gAlpha;
    float4x4 gViewProjMatrix;
    bool gForceReuse;
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

bool checkDiscard(int2 vPos, int2 vPrevPos)
{
    // check if position is acceptable
    if (gEnableDiscardByPosition)
    {
        float3 CurPos = gTexCurPos[vPos].xyz;
        float4 CurPosH = mul(float4(CurPos, 1.0), gViewProjMatrix);
        float Depth = CurPosH.z / CurPosH.w;
        float3 PrevPos = gTexPrevPos[vPrevPos].xyz;
        float MaxOffset = pow(Depth, 2) * 0.05 + 0.01; // further => larger acceptable offset
        if (distance(CurPos, PrevPos) > MaxOffset)
            return true;
    }
        
    // check if normal is acceptable
    if (gEnableDiscardByNormal)
    {
        float3 CurNormal = gTexCurNormal[vPos].xyz;
        float3 PrevNormal = gTexPrevNormal[vPrevPos].xyz;
        if (distance(CurNormal, PrevNormal) > 0.1)
            return true;
    }

    return false;
}

float clampVisibility(int2 vPos, float vVal)
{
    float MinV = 1.0, MaxV = 0.0;
    int r = int(gClampSearchRadius);
    for (int i = -r; i <= r; ++i)
    {
        for (int k = -r; k <= r; ++k)
        {
            float NearV = gTexVisibility[vPos + float2(i, k)].r;
            MinV = min(MinV, NearV);
            MaxV = max(MaxV, NearV);
        }
    }
    const float Padding = gClampExtendRange;
    return clamp(vVal, MinV - Padding, MaxV + Padding); // simple clamp
}

float calcBlendAlpha(int2 vPos, float2 vMotionVector)
{
    if (gForceReuse)
    {
        return gAlpha;
    }
    
    if (gAdaptiveAlpha)
    {
        float Variation = saturate(gTexAlpha[vPos].r);
        float v = (gReverseVariation ? (1 - Variation) : Variation);
        if (true) // smooth
        {
            return gAlpha * v;
        }
        else // threshold
        {
            // smooth threshold
            float Min = 0.05, Max = 0.15;
            float tempV = clamp(Variation, Min, Max);
            tempV = (tempV - Min) / (Max - Min);
            return gAlpha * (gReverseVariation ? (1 - tempV) : tempV);
            //if ((v < 0.1 && !gReverseVariation) || (gReverseVariation && v < 0.9))
            //    return 0.0;
        }
    }

    return gAlpha;
}

psOut main(float2 texC : TEXCOORD, float4 posH : SV_POSITION) : SV_Target
{
    float2 pixelPos = posH.xy;
    int2 iPos = int2(pixelPos);
    uint2 texDim;
    uint levels;
    gTexVisibility.GetDimensions(0, texDim.x, texDim.y, levels);
    const float2 imageDim = float2(texDim);
    
    float currentV = gTexVisibility[iPos].r;

    float resultV = currentV;
    float4 debug = float4(0.0);
    if (gEnableBlend)
    {
        float2 motionVector = gTexMotionVector[iPos].xy;
        const float2 iPosPrev = float2(iPos) + motionVector.xy * imageDim + float2(0.5, 0.5);
        if (iPosPrev.x > 0 && iPosPrev.y > 0 && iPosPrev.x < texDim.x && iPosPrev.y < texDim.y)
        {
            const float alpha = calcBlendAlpha(iPos, motionVector);
            float prevoiusV = gTexPrevVisiblity[iPosPrev].r;
            
            // clamp
            if (gEnableClamp)
                prevoiusV = clampVisibility(iPos, prevoiusV);
        
            bool discardPrev = checkDiscard(iPos, iPosPrev);
            // debug output
            //debug = float4(Clamped ? 1.0 : 0.0, discardPrev ? 1.0 : 0.0, 0.0, 1.0);
            //debug = float4(Clamped ? 1.0 : 0.0, discardByPosition ? 1.0 : 0.0, discardByNormal ? 1.0 : 0.0, 1.0);
            debug = float4(float3(alpha / gAlpha), 1.0);

            if (!discardPrev)
                resultV = lerp(prevoiusV, currentV, alpha);
        }
    }
    
    psOut Output;
    Output.Result = float4(resultV, resultV, resultV, 1.0);
    Output.Debug = debug;
    return Output;
}
