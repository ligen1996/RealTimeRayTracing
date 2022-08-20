#include "Discard.slang"
#define _SAMPLE_TYPE_POINT 0u
#define _SAMPLE_TYPE_BLINEAR 1u
#define _SAMPLE_TYPE_CATMULL 2u

SamplerState gSamplerLinear;

float2 getPrevPos(float2 vTexCoord, float2 vMotionVector, int2 vTexDim)
{
    return (vTexCoord + vMotionVector) * vTexDim;
    
    //const float2 PosPrev = float2(iPos) + vMotionVector.xy * vTexDim + float2(0.5, 0.5);
    //const float2 PosPrev = floor((vTexCoord + vMotionVector) * vTexDim) + 0.5;
}

bool getPrevPoint(Texture2D vPrevTex, float2 vPos, float2 vPrevPos, int2 vTexDim, float4x4 vViewProjMatrix, float vPosStrength, float vNormalStrength, out float4 voRes)
{
    // test, no interpolate, but bamboo artifact
    if (isValid(vPos, vPrevPos, vTexDim, vViewProjMatrix, vPosStrength, vNormalStrength))
    {
        voRes = vPrevTex[vPrevPos];
        return true;
    }

    return false;
}

bool getPrevBilinear(Texture2D vPrevTex, float2 vPos, float2 vPrevPos, int2 vTexDim, float4x4 vViewProjMatrix, float vPosStrength, float vNormalStrength, out float4 voRes)
{
    // use sampler to interpolate, dont use, it cause artifacts on screen edge
    // need to upload a linear sampler
    //if (isValid(vPos, vPrevPos, vTexDim, vViewProjMatrix, vPosStrength, vNormalStrength))
    //{
    //    voRes = vPrevTex.Sample(gSamplerLinear, vPrevPos / float2(vTexDim));
    //    return true;
    //}
    //return false;
    
    vPrevPos -= 0.5; // ??? why ???
    // self-implemented interpolate with validation
    float x = frac(vPrevPos.x);
    float y = frac(vPrevPos.y);

    // bilinear weights
    const float w[4] =
    {
        (1 - x) * (1 - y),
             x * (1 - y),
        (1 - x) * y,
             x * y
    };
    const int2 offset[4] = { int2(0, 0), int2(1, 0), int2(0, 1), int2(1, 1) };
    
    // perform the actual bilinear interpolation
    float sumw = 0;
    float SumPrev = 0.0;
    for (int sampleIdx = 0; sampleIdx < 4; sampleIdx++)
    {
        const int2 CurPrevPos = int2(vPrevPos) + offset[sampleIdx];
        if (isValid(vPos, CurPrevPos, vTexDim, vViewProjMatrix, vPosStrength, vNormalStrength))
        {
            SumPrev += w[sampleIdx] * vPrevTex[CurPrevPos].r;
            sumw += w[sampleIdx];
        }
    }
    
    // redistribute weights in case not all taps were used
    bool valid = (sumw >= 0.01);
    voRes = valid ? SumPrev / sumw : 0;
    return valid;
}

bool getPrevCatmull(Texture2D vPrevTex, float2 vPos, float2 vPrevPos, int2 vTexDim, float4x4 vViewProjMatrix, float vPosStrength, float vNormalStrength, out float4 voRes)
{
    float2 invTextureSize = 1.0 / vTexDim;
    vPrevPos -= 0.5;
    float2 f = frac(vPrevPos);
    float2 f2 = f * f;
    float2 f3 = f2 * f;
    
    float2 w0 = f2 - 0.5 * (f3 + f);
    float2 w1 = 1.5 * f3 - 2.5 * f2 + 1;
    float2 w3 = 0.5 * (f3 - f2);
    float2 w2 = 1 - w0 - w1 - w3;

    float2 w12 = w1 + w2;
    
    float2 Pos0 = (vPrevPos - 1);
    float2 Pos12 = (vPrevPos + w2 / w12);
    float2 Pos3 = (vPrevPos + 1);

    float2 Ps[3] = { Pos0, Pos12, Pos3 };
    float2 Ws[3] = { w0, w12, w3 };

    const int2 Offsets[9] =
    {
        int2(0, 0),
        int2(0, 1),
        int2(0, 2),
        int2(1, 0),
        int2(1, 1),
        int2(1, 2),
        int2(2, 0),
        int2(2, 1),
        int2(2, 2),
    };
    
    float4 SumPrev = 0.0;
    float SumW = 0.0;
    for (int i = 0; i < 9; i++)
    {
        int i1 = Offsets[i].x, i2 = Offsets[i].y;
        float2 Pa = Ps[i1], Pb = Ps[i2];
        float2 wa = Ws[i1], wb = Ws[i2];
        float2 TexCoord = float2(Pa.x, Pb.y);
        float w = (wa.x * wb.y);
        
        if (isValid(vPos, TexCoord, vTexDim, vViewProjMatrix, vPosStrength, vNormalStrength))
        {
            //float4 Color = vPrevTex[TexCoord];
            float4 Color = vPrevTex.Sample(gSamplerLinear, TexCoord / vTexDim); // still used sample
            SumPrev += Color * w;
            SumW += w;
        }
    }

    bool valid = (SumW >= 0.01);
    voRes = valid ? SumPrev / SumW : float4(0.0);
    return valid;
}

bool getPrev(int vType, Texture2D vPrevTex, float2 vPos, float2 vPrevPos, int2 vTexDim, float4x4 vViewProjMatrix, float vPosStrength, float vNormalStrength, out float4 voRes)
{
    switch (vType)
    {
        case _SAMPLE_TYPE_POINT:
            return getPrevPoint(vPrevTex, vPos, vPrevPos, vTexDim, vViewProjMatrix, vPosStrength, vNormalStrength, voRes);
        case _SAMPLE_TYPE_CATMULL:
            return getPrevCatmull(vPrevTex, vPos, vPrevPos, vTexDim, vViewProjMatrix, vPosStrength, vNormalStrength, voRes);
        case _SAMPLE_TYPE_BLINEAR:
        default:
            return getPrevBilinear(vPrevTex, vPos, vPrevPos, vTexDim, vViewProjMatrix, vPosStrength, vNormalStrength, voRes);
    }
    return false;
}
