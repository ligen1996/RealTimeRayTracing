#define _USE_MOTION_VECTOR
#define _ACCURACY 0.01
#define _ENABLE_CLAMP
#define _ENABLE_DISCARD_BY_POSITION
#define _ENABLE_DISCARD_BY_NORMAL

cbuffer PerFrameCB : register(b0)
{
    bool gEnableBlend;
    float gAlpha;
    uint2 gRenderTargetDim;
    float4x4 gViewProjMatrix;
}

Texture2D gTexMotionVector;
Texture2D gTexVisibility;
Texture2D gTexPrevVisiblity;
Texture2D gTexCurPos;
Texture2D gTexPrevPos;
Texture2D gTexCurNormal;
Texture2D gTexPrevNormal;

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
    
        if (iposPrev.x > 0 && iposPrev.y > 0 && iposPrev.x < texDim.x && iposPrev.y < texDim.y)
        {
            bool discardPrev = false;
            float prevoiusV = gTexPrevVisiblity[iposPrev].r;

            // clamp
            // simple clamp seems not work on multiview soft shadow
            // as the shadow flicks method produces a wrong clamp on penumbra
            bool Clamped = false;
#ifdef _ENABLE_CLAMP
            float minV = 1.0, maxV = 0.0;
            for (int i = -2; i <= 2; ++i)
            {
                for (int k = -2; k <= 2; ++k)
                {
                    float nearV = gTexVisibility[iPos + float2(i, k)].r;
                    minV = min(minV, nearV);
                    maxV = max(maxV, nearV);
                }
            }
            const float Padding = 0.05;
            if (prevoiusV < minV - Padding - _ACCURACY || prevoiusV > maxV + Padding + _ACCURACY)
            {
                Clamped = true;
            }
            prevoiusV = clamp(prevoiusV, minV - Padding, maxV + Padding); // simple clamp
            //prevoiusV = lerp(prevoiusV, (minV + maxV) * 0.5, 0.0001); // not clamp but slightly adjust previous value
#endif
        
            // check if position is acceptable
            bool discardByPosition = false;
#ifdef _ENABLE_DISCARD_BY_POSITION
            float3 CurPos = gTexCurPos[iPos].xyz;
            float4 CurPosH = mul(float4(CurPos, 1.0), gViewProjMatrix);
            float Depth = CurPosH.z / CurPosH.w;
            float3 PrevPos = gTexPrevPos[iposPrev].xyz;
            float MaxOffset = pow(Depth, 2) * 0.05 + 0.01; // further => larger acceptable offset
            if (distance(CurPos, PrevPos) > MaxOffset)
            {
                discardPrev = discardByPosition = true;
            }
#endif
        
            // check if normal is acceptable
            bool discardByNormal = false;
#ifdef _ENABLE_DISCARD_BY_NORMAL
            float3 CurNormal = gTexCurNormal[iPos].xyz;
            float3 PrevNormal = gTexPrevNormal[iposPrev].xyz;
            if (distance(CurNormal, PrevNormal) > 0.1)
            {
                discardPrev = discardByNormal = true;
            }
#endif
        
            // debug output
            //debug = float4(Clamped ? 1.0 : 0.0, discardPrev ? 1.0 : 0.0, 0.0, 1.0);
            debug = float4(Clamped ? 1.0 : 0.0, discardByPosition ? 1.0 : 0.0, discardByNormal ? 1.0 : 0.0, 1.0);

            if (!discardPrev)
                resultV = lerp(prevoiusV, currentV, gAlpha);
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
