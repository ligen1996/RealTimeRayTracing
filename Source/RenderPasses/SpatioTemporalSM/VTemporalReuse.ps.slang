import Scene.ShadingData;
import Utils.Math.MathHelpers;
import Utils.Color.ColorHelpers;

cbuffer PerFrameCB : register(b0)
{
    float gAlpha;
    uint2 gRenderTargetDim;
}

Texture2D gTexMotionVector;
Texture2D gTexVisibility;
Texture2D gTexPrevVisiblity;
//todo motion vector

float4 main(float2 texC : TEXCOORD, float4 posH : SV_POSITION) : SV_Target
{
    float2 pixelPos = posH.xy;
    int2 iPos = int2(pixelPos);
    uint2 texDim;
    uint levels;
    gTexVisibility.GetDimensions(0, texDim.x, texDim.y, levels);
    const float2 imageDim = float2(texDim);
    
    float currentV = gTexVisibility[iPos].r;
    float2 motionVector = gTexMotionVector[iPos].xy;
    const float2 iposPrev = float2(iPos) + motionVector.xy * imageDim + float2(0.5, 0.5);

    float resultV = currentV;
    if (iposPrev.x > 0 && iposPrev.y > 0 && iposPrev.x < texDim.x && iposPrev.y < texDim.y)
    {
        bool discardPrev = false;
        float prevoiusV = gTexPrevVisiblity[iposPrev].r;

        // clamp
        float minV = 1.0, maxV = 0.0;
        for (int i = -1; i <= 1; ++i)
        {
            for (int k = -1; k <= 1; ++k)
            {
                float nearV = gTexPrevVisiblity[iposPrev + float2(i, k)].r;
                minV = min(minV, nearV);
                maxV = max(maxV, nearV);
            }
        }
        prevoiusV = clamp(prevoiusV, minV, maxV);

        if (!discardPrev)
            resultV = lerp(prevoiusV, currentV, gAlpha); //todo adjust alpha
    }
    
    //float resultV = currentV;

    return float4(resultV);
}
