


cbuffer PerFrameCB : register(b0)
{
    int gKernelRadius;
    float gSigmaCoord;
    float gSigmaColor;
    float gSigmaPlane;
    float gSigmaNormals;
}

Texture2D gVTex;
Texture2D gNormalTex;
Texture2D gPositionTex;
//Texture2D gDepthTex;


struct psOut
{
    float4 filterV : SV_Target0;
};

//todo:need to be optimize
float JointBilateralFilterKernel(float DistanceSqr,float ColorDistSqr,float NormalDistSqr,float PositionDistSqr,
                                 float SigmaD,float SigmaC,float SigmaN,float SigmaP)
{
    return exp(-(DistanceSqr / (2 * SigmaD * SigmaD)) - (ColorDistSqr / (2 * SigmaC * SigmaC)) -
               (NormalDistSqr / (2 * SigmaN * SigmaN)) - (PositionDistSqr / (2 * SigmaP * SigmaP)));
}

float DistanceSqr(uint ix,uint iy,uint jx,uint jy)
{
    return float(pow((ix - jx), 2) + pow((iy - jy), 2));
}


psOut main(float2 texC : TEXCOORD, float4 posH : SV_POSITION)
{
    //todo:
    float2 PixelPostion = posH.xy;
    uint2 pixelPos = uint2(PixelPostion);
    
    uint2 texDim;
    uint levels;
    int kernelRadius = gKernelRadius;

    gVTex.GetDimensions(0, texDim.x, texDim.y, levels);

    int x = pixelPos.x;
    int y = pixelPos.y;
    
    int height = texDim.y;
    int width = texDim.x;
    
    float3 filteredPos = gPositionTex[pixelPos].rgb;
    float3 filteredNormal = gNormalTex[pixelPos].rgb;
    filteredNormal = normalize(filteredNormal);
    float Visibility = gVTex[pixelPos].r;

    float FinalRes = 0.0f;
    float weight = 0.0; // weight_xy sum
    for (int i_y = y - kernelRadius; i_y <= y + kernelRadius; ++i_y)
    {
        for (int i_x = x - kernelRadius; i_x <= x + kernelRadius; ++i_x)
        {
            float w_ixy = 0.0;
				// Out of Bounds
            if (i_y < 0 || i_y >= height || i_x < 0 || i_x >= width)
            {
                continue;
            }
            else
            {
                int2 curPixelPos = int2(i_x, i_y);
                float3 CurNormal = normalize(gNormalTex[curPixelPos].rgb);
                float3 CurPos = gPositionTex[curPixelPos].rgb;
                float curV = gVTex[curPixelPos].r;

                float DisSqr = DistanceSqr(x, y, i_x, i_y);
                
                float colorDistSqr = (curV - Visibility) * (curV - Visibility);
                
                float NormDot = dot(filteredNormal, CurNormal);
                NormDot = clamp(NormDot, 0.0f, 1.0f);

                float DnormalSqr = pow((acos(NormDot)), 2);
                
                float3 DistancePos = normalize(CurPos - filteredPos);
                
                float Dplane = dot(filteredNormal, DistancePos);
                Dplane = clamp(Dplane, 1e-5f, 1.0f);

                w_ixy = JointBilateralFilterKernel(DisSqr, colorDistSqr, DnormalSqr, Dplane,
									                gSigmaCoord, gSigmaColor, gSigmaNormals, gSigmaPlane);
                
                FinalRes += curV * w_ixy;
					// add weights
                weight += w_ixy;
            }
        }
    }
    if (0.0 != weight)
    {
        FinalRes /= weight;
    }
    else
    {
        FinalRes = 0.0;
    }

   

    psOut psRes;
    psRes.filterV = float4(FinalRes);
    
    return psRes;
}
