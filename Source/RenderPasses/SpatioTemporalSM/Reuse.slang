#include "Discard.slang"

float getPrev(Texture2D vTex, float2 vPos, float2 vPrevPos, int2 vTexDim, float4x4 vViewProjMatrix, float vPosStrength, float vNormalStrength)
{
    if (isValid(vPos, vPrevPos, vTexDim, vViewProjMatrix, vPosStrength, vNormalStrength))
    {
        return vTex[vPrevPos].r;
    }

    return 0.0;
}

float getPrevBilinear(Texture2D vTex, float2 vPos, float2 vPrevPos, int2 vTexDim, float4x4 vViewProjMatrix, float vPosStrength, float vNormalStrength)
{
    float sumw = 0;
    float x = frac(vPrevPos.x);
    float y = frac(vPrevPos.y);

    // bilinear weights
    const float w[4] =
    {
        (1 - x) * (1 - y),
             x  * (1 - y),
        (1 - x) *      y,
             x  *      y
    };
    const int2 offset[4] = { int2(0, 0), int2(1, 0), int2(0, 1), int2(1, 1) };
    
    // perform the actual bilinear interpolation
    float SumPrev = 0.0;
    for (int sampleIdx = 0; sampleIdx < 4; sampleIdx++)
    {
        const int2 CurPrevPos = int2(vPrevPos) + offset[sampleIdx];
        if (isValid(vPos, CurPrevPos, vTexDim, vViewProjMatrix, vPosStrength, vNormalStrength))
        {
            SumPrev += w[sampleIdx] * vTex[CurPrevPos].r;
            sumw += w[sampleIdx];
        }
    }
    
    // redistribute weights in case not all taps were used
    bool valid = (sumw >= 0.01);
    return valid ? SumPrev / sumw : 0;
}

