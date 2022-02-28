Texture2D gR;
Texture2D gG;
Texture2D gB;
Texture2D gA;

float4 main(float2 texC : TEXCOORD) : SV_Target0
{
    uint2 texDim;
    uint levels;
    gR.GetDimensions(0, texDim.x, texDim.y, levels);

    float2 pixelPos = texC * texDim;
    int2 iPos = int2(pixelPos);
    
    float r = gR[iPos].x;
    float g = gG[iPos].x;
    float b = gB[iPos].x;
    float a = gA[iPos].x;

    //return float4(r,r,r,1);
    return float4(r, g, b, 1.);
}
