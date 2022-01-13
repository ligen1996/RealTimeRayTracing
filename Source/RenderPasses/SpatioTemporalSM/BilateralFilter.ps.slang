#define _MAX_KERNEL_SIZE 31

cbuffer PerFrameCB : register(b0)
{
    bool gEnable = true;
    uint gDirection = 0u;
    float gSigmaColor = 10.0;
    float gSigmaNormal= 10.0;
    float gSigmaDepth = 10.0;
    uint gKernelSize = 15;
}

Texture2D gTexColor;
Texture2D gTexNormal;
Texture2D gTexDepth;

float luminance(float3 rgb)
{
    return 0.2126 * rgb.r + 0.7152 * rgb.g + 0.0722 * rgb.b;
}

float gaussian(float x, float sigma)
{
    return 0.39894 * exp(-0.5 * x * x / (sigma * sigma)) / sigma;
}

float deltaDepth(int2 vTexCoordInt, uint vDirection)
{
    float CurDepth = gTexDepth[vTexCoordInt].r;
    if (vDirection == 0u)
    {
        float NextDepth = gTexDepth[vTexCoordInt + int2(1, 0)].r;
        return NextDepth - CurDepth;
    }
    else
    {
        float NextDepth = gTexDepth[vTexCoordInt + int2(0, 1)].r;
        return NextDepth - CurDepth;
    }
}

// direction == 0, x方向滤波，否则y方向；
float3 separateJBFPassKernel(int2 vTexCoordInt, const uint2 vTexSize, uint vDirection)
{
    const float3 shadingCenter = gTexColor[vTexCoordInt].xyz;
    if (!gEnable)
        return shadingCenter;
    
    const float3 normalCenter = gTexNormal[vTexCoordInt].xyz;
    const float depthCenter = gTexDepth[vTexCoordInt].r;
    const float phiSpace = (float) gKernelSize / 3.0f;
    int dd = (gKernelSize - 1) / 2;
    
    const float luminanceCenter = luminance(shadingCenter);
    float sumWeightS = gaussian(0.0f, phiSpace);
    float3 sumShading = shadingCenter * sumWeightS;
	[unroll(dd)]
    for (int i = -dd; i <= dd; i++)
    {
        int2 SampleTexCoordInt = vTexCoordInt;
        if (vDirection == 0u)
        {
            SampleTexCoordInt.x += i;
        }
        else
        {
            SampleTexCoordInt.y += i;
        }
        
        float DeltaDepth = deltaDepth(SampleTexCoordInt, vDirection);
        
        if (SampleTexCoordInt.x < 0 || SampleTexCoordInt.y < 0 || SampleTexCoordInt.x >= vTexSize.x || SampleTexCoordInt.y >= vTexSize.y)
            continue;
        if (i == 0)
            continue;
        const float3 shadingP = gTexColor[SampleTexCoordInt].rgb;
        const float luminanceP = luminance(shadingP);
        const float3 normalP = gTexNormal[SampleTexCoordInt].xyz;
        const float depthP = gTexDepth[SampleTexCoordInt].r;

        const float dist2Space = i * i;
        const float weightSpace = gaussian(dist2Space, phiSpace);

        float weightC = luminanceCenter - luminanceP;
        weightC = abs(weightC);
        weightC /= gSigmaColor;
        weightC = max(weightC, 0.0f); 
        weightC = exp(-weightC);

        float weightN = dot(normalCenter, normalP);
        weightN = saturate(weightN);
        weightN = pow(weightN, gSigmaNormal);
        weightN = max(0.0f, weightN);

        const float expectedNeighborDepth = depthCenter + DeltaDepth * (float) (i);
        const float neighborDepthError = abs(expectedNeighborDepth - depthP);
        const float expectedDifference = abs(expectedNeighborDepth - depthCenter) + 1e-8f;
        float weightD = neighborDepthError / (expectedDifference * gSigmaDepth); // 1
        weightD = max(weightD, 0.0f);
        weightD = exp(-weightD);

        const float weightS = weightD * weightC * weightN * weightSpace;
        sumShading += shadingP * weightS;
        sumWeightS += weightS;
    }
    sumShading /= sumWeightS;
    return sumShading;
}

float4 main(float2 texC : TEXCOORD, float4 posH : SV_POSITION) : SV_Target
{
    uint3 Dim;
    gTexColor.GetDimensions(0, Dim.x, Dim.y, Dim.z);
    int2 TexCoordInt = int2(texC.x * Dim.x + 0.5, texC.y * Dim.y + 0.5);

    if (gDirection <= 1u)
        return float4(separateJBFPassKernel(TexCoordInt, Dim.xy, gDirection), 1.0);
    else
        return 0.5 * (float4(separateJBFPassKernel(TexCoordInt, Dim.xy, 0u), 1.0) + float4(separateJBFPassKernel(TexCoordInt, Dim.xy, 1u), 1.0));
}
