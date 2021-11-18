
Texture2D gTexColorIn;
Texture2D gTexVisibilityIn;


float4 main(float2 texC:TEXCOORD):SV_Target0
{
    uint2 texDim;
    uint levels;
    gTexColorIn.GetDimensions(0, texDim.x, texDim.y, levels);

    float2 pixelPos = texC * texDim;
    int2 iPos = int2(pixelPos);
    
    float3 color = gTexColorIn[iPos].xyz; //todo sampler it
    float visibility = gTexVisibilityIn[iPos].x;

    return float4(color * visibility, 1.0f);
}
