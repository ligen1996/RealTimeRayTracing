#include "ReuseFactorEstimationConstant.slangh"

cbuffer PerFrameCB : register(b0)
{
    uint gMapType;
    float gParam1;
    float gParam2;
}

Texture2D<float> gTexValue;

float mapCurve(float V)
{
    //V = pow(2, pow(V, 2)) - 1;
    V = saturate((V - gParam1) / (gParam2 - gParam1));
    return V;
}

float main(float2 uvs : TEXCOORD, float4 pos : SV_POSITION) : SV_TARGET0
{
    float V = gTexValue[pos.xy].r;

    if (gMapType == _MAP_TYPE_CURVE)
    {
        return mapCurve(V);
    }
    else
    {
        return 0.5;
    }
}
