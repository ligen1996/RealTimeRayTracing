


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
    return exp(-(DistanceSqr / (2 * sqrt(SigmaD))) - (ColorDistSqr / (2 * sqrt(SigmaC))) -
               (NormalDistSqr / (2 * sqrt(SigmaN))) -(PositionDistSqr / (2 * sqrt(SigmaP))));
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

    float m_sigmaCoord = gSigmaCoord;
    float m_sigmaColor = gSigmaColor;
    float m_sigmaPlane = gSigmaPlane;
    float m_sigmaNormals = gSigmaNormals;
    

    int x_start = max(0, x - kernelRadius);
    int x_end = min(width - 1, x + kernelRadius);
    int y_start = max(0, y - kernelRadius);
    int y_end = min(height - 1, y + kernelRadius);

    float weight = 0.0;
    float resColor;
    float weight_sum = 0.0f;

    float3 center_postion = gPositionTex[pixelPos].rgb;
    float3 center_normal = gNormalTex[pixelPos].rgb;
    float center_color = gVTex[pixelPos].r;


   for (int m = x_start; m <= x_end; m++)
   {
       for (int n = y_start; n <= y_end; n++)
        {
            if (n == x && y == m)
            {
                weight_sum += 1.0f;
                resColor += center_color;
                continue;
            }
       

            int2 curPixelPos = int2(m, n);
            float3 curr_postion = gPositionTex[curPixelPos].rgb;
            float3 curr_normal = gNormalTex[curPixelPos].rgb;
            float curr_color = gVTex[curPixelPos].r;
            
            float plane_dis = .0f;
            float position_dis = distance(center_postion, curr_postion) / (2.0f * m_sigmaCoord);
            float color_dis = abs(center_color - curr_color) / (2.0f * m_sigmaColor);
            float normalDot = dot(center_normal, curr_normal);
            float normal_dis = clamp(normalDot, 0.0, 1.0) / (2.0f * m_sigmaNormals);
        
            if (position_dis != 0.f)
            {
                plane_dis = dot(center_normal, normalize(curr_postion - center_postion));
            }
        
            plane_dis = plane_dis * plane_dis / (2.0f * m_sigmaPlane);
            float weight = exp(-(plane_dis + position_dis + color_dis + normal_dis));
            weight_sum += weight;
            resColor += curr_color * weight;
        }
    }
    
    float Res = resColor / weight_sum;

    psOut psRes;
    psRes.filterV = float4(Res);
    
    return psRes;
}
