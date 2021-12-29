#define _MAX_KERNEL_SIZE 31

cbuffer PerFrameCB : register(b0)
{
    bool gEnable = true;
    float gSigma = 10.0;
    float gBSigma = 0.1;
    uint gMSize = 15;
}

Texture2D gTexInput;

float normpdf(float x, float sigma)
{
    return 0.39894 * exp(-0.5 * x * x / (sigma * sigma)) / sigma;
}

float normpdf3(float3 v, float sigma)
{
    return 0.39894 * exp(-0.5 * dot(v, v) / (sigma * sigma)) / sigma;
}

float4 main(float2 texC : TEXCOORD, float4 posH : SV_POSITION) : SV_Target
{
    uint Width, Height, LevelNum;
    gTexInput.GetDimensions(0, Width, Height, LevelNum);

    int2 TexCoordInt = int2(texC.x * Width + 0.5, texC.y * Height + 0.5);
    float4 CurColor = gTexInput[TexCoordInt];
    if (!gEnable)
    {
        return CurColor;
    }
    
	//declare stuff
    const int kSize = (gMSize - 1) / 2;
    float kernel[_MAX_KERNEL_SIZE];
    float4 SumColor = float4(0.0);
	
	//create the 1-D kernel
    for (int i = 0; i <= kSize; ++i)
    {
        kernel[kSize + i] = kernel[kSize - i] = normpdf(float(i), gSigma);
    }
	
    float bZ = 1.0 / normpdf(0.0, gBSigma);
    float SumWeight = 0.0;
	//read out the texels
    for (int i = -kSize; i <= kSize; ++i)
    {
        for (int j = -kSize; j <= kSize; ++j)
        {
            float4 Color = gTexInput[TexCoordInt + int2(i, j)];
            float Weight = normpdf3(Color.rgb - CurColor.rgb, gBSigma) * bZ * kernel[kSize + j] * kernel[kSize + i];
            SumWeight += Weight;
            SumColor += Weight * Color;
        }
    }
	
    return float4(SumColor / SumWeight);
}
