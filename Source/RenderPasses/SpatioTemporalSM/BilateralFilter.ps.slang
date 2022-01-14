#include "BilateralFilterCommon.slang"

float4 main(float2 texC : TEXCOORD, float4 posH : SV_POSITION) : SV_Target
{
    uint3 Dim;
    gTexColor.GetDimensions(0, Dim.x, Dim.y, Dim.z);
    int2 TexCoordInt = int2(texC.x * Dim.x, texC.y * Dim.y);

    return float4(separateJBFPassKernel(TexCoordInt, Dim.xy), 1.0);
}
