#include "BlendAlpha.slang"
#include "Reuse.slang"
#define _ACCURACY 0.01

cbuffer PerFrameCB : register(b0)
{
    uint gReuseSampleType = 0u;
    bool gEnableBlend;
    bool gEnableClamp = true;
    uint gClampSearchRadius = 1;
    float gClampExtendRange = 0.0;
    float gDiscardByPositionStrength = 1.0f;
    float gDiscardByNormalStrength = 1.0f;
    bool gAdaptiveAlpha = true;
    float gAlpha;
    float gBeta;
    float4x4 gViewProjMatrix;
    float gRatiodv = 0.2f;
    float gRatioddv = 1.0f;
}

Texture2D gTexMotionVector;
Texture2D gTexVisibility;
Texture2D gTexPrevVisiblity;
Texture2D<float> gTexPrevVariation;
Texture2D<float> gTexPrevVarOfVar;

struct psOut
{
    float4 Result : SV_Target0;
    float4 Debug : SV_Target1;
};

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

float calcBlendAlpha(int2 vPrevPos/*, bool vForceAdaptive = false*/)
{
    if (gAdaptiveAlpha /* || vForceAdaptive */)
    {
        // 1. full bright or full dark: small dv, small ddv, big alpha to avoid ghosting
        // 2. half bright/dark: large dv, small ddv, small alpha to hasten covergence
        // 3. edge of 1 and 2: dont care dv, big ddv, big alpha to avoid ghosting
        // 4. edge of object: big dv, big ddv, big alpha to avoid ghosting (discard by pos/normal can handle part of this)
        // have to design a 2d function that satisfy above conditions
        float dv = saturate(gTexPrevVariation[vPrevPos].r);
        float ddv = saturate(gTexPrevVarOfVar[vPrevPos].r);
        //return calAlpha(dv, ddv, gAlpha, gRatiodv, gRatioddv);
        return interpAlpha(dv, ddv, gAlpha , gBeta, gRatiodv, gRatioddv);
        //return calcBlendAlphaCommon(dv, ddv, gAlpha, gRatiodv, gRatioddv);
    }

    return gAlpha;
}

float calNonPlanarAlpha(float vAlpha,float2 vImagePos, float2 vImagePrePos)
{
    float3 Differ = gTexPrevPos[vImagePrePos].xyz - gTexCurPos[vImagePos].xyz;
    float3 Normal = gTexCurNormal[vImagePrePos].xyz;
    float Cos = dot(normalize(Differ), normalize(Normal));
    float FallOff = exp((-(1 - Cos) * (1 - Cos)) / (0.02));
    return clamp(1 - FallOff * (1. - vAlpha), vAlpha, 1.);
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
        const float2 PosPrev = getPrevPos(texC, motionVector, texDim);
        float4 TempPrev = float4(0.0);
        bool valid = getPrev(gReuseSampleType, gTexPrevVisiblity, iPos, PosPrev, texDim, gViewProjMatrix, gDiscardByPositionStrength, gDiscardByNormalStrength, TempPrev);
        
        //float DebugAdaptiveAlpha = calcBlendAlpha(PosPrev, true);
        //debug = float4(float3(DebugAdaptiveAlpha), 1.0);
        if (valid)
        {
            float prevoiusV = TempPrev.r;
            float alpha = calcBlendAlpha(PosPrev);
            
            // clamp
            //if (gEnableClamp)
            //    prevoiusV = clampVisibility(iPos, prevoiusV);
            
            resultV = lerp(prevoiusV, currentV, alpha);
        }
    }
    
    psOut Output;
    Output.Result = float4(resultV, resultV, resultV, 1.0);
    //Output.Debug = debug;
    return Output;
}
