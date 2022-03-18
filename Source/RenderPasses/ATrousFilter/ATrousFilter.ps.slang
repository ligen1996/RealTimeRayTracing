#pragma once
#include "..\ATrousFilter\PassData.slangh"

Texture2D gVisibility;
Texture2D gLinearZ;
Texture2D gNormal;
Texture2D gDV;
//Texture2D gDDV;

cbuffer PerFrameCB
{
    int gStepSize;
    SPassData g;
};

struct FullScreenPassVsOut
{
    float2 texC : TEXCOORD;
    float4 posH : SV_POSITION;
};

int2 getTextureDims(Texture2D tex, uint mip)
{
    uint w, h;
    tex.GetDimensions(w, h);
    return int2(w, h);
}

float computeVarianceCenter(int2 ipos)
{
    float sum = 0.f;

    const float kernel[2][2] =
    {
        { 1.0 / 4.0, 1.0 / 8.0 },
        { 1.0 / 8.0, 1.0 / 16.0 }
    };

    const int radius = 1;
    for (int yy = -radius; yy <= radius; yy++)
    {
        for (int xx = -radius; xx <= radius; xx++)
        {
            const int2 p = ipos + int2(xx, yy);
            const float k = kernel[abs(xx)][abs(yy)];
            sum += gVisibility.Load(int3(p, 0)).a * k;
        }
    }

    return sum;
}

float computeWeight(
    float zCenter, float zP, float phiZ,
    float3 nCenter, float3 nP, float phiN,
    float vCenter, float vP, float phiV)
{
    const float weightNormal = max(0, pow(saturate(dot(nCenter, nP)), phiN));
    const float weightZ = (phiZ == 0) ? 0.0f : abs(zCenter - zP) / phiZ;
    const float weightLillum = abs(vCenter - vP) / phiV;

    const float weight = exp(0.0 - max(weightLillum, 0.0) - max(weightZ, 0.0)) * weightNormal;

    return weight;
}

float4 main(FullScreenPassVsOut vsOut) : SV_TARGET0
{
    const int2 ipos = int2(vsOut.posH.xy);
    const int2 screenSize = getTextureDims(gVisibility, 0);

    const float kernelWeights[3] = { 1.0, 2.0 / 3.0, 1.0 / 6.0 };

    float vCenter = gVisibility.Load(int3(ipos, 0)).r;
    float2 zCenter = gLinearZ[ipos].xy;
    if (zCenter.x < 0)
    {
        // not a valid depth => must be envmap => do not filter
        return vCenter;
    }
    float3 nCenter = gNormal[ipos].xyz;

    const float phiV = g.PhiVisibility * sqrt(max(0.0, 1e-10 + computeVarianceCenter(ipos).r));
    const float phiZ = max(zCenter.y, 1e-8) * gStepSize;
    const float phiN = g.PhiNormal;

    float sumW = 1.0;
    float sum = vCenter;

    for (int yy = -2; yy <= 2; yy++)
    {
        for (int xx = -2; xx <= 2; xx++)
        {
            const int2 p = ipos + int2(xx, yy) * gStepSize;
            const bool inside = all(p >= int2(0, 0)) && all(p < screenSize);

            const float kernel = kernelWeights[abs(xx)] * kernelWeights[abs(yy)];

            if (inside && (xx != 0 || yy != 0)) // skip center pixel, it is already accumulated
            {
                const float vP = gVisibility.Load(int3(p, 0)).x;
                const float zP = gLinearZ[p].x;
                const float3 nP = gNormal[p].xyz;

                // compute the edge-stopping functions
                const float2 w = computeWeight(
                    zCenter.x, zP, phiZ * length(float2(xx, yy)),
                    nCenter, nP, phiN,
                    vCenter, vP, phiV);

                const float W = w.x * kernel;

                // alpha channel contains the variance, therefore the weights need to be squared, see paper for the formula
                sumW += W;
                sum += W * vP;
            }
        }
    }

    float Filtered = sum / sumW;
    return float4(float3(Filtered), 1.);
    //return float4(0.6, 0.8, 0.5, 1);
}
