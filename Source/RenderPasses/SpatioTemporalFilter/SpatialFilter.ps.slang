

Texture2D gVTex;
Texture2D gNormalTex;
Texture2D gPositionTex;
Texture2D gDepthTex;


struct psOut
{
    
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


psOut main()
{
    //todo:
    uint2 pixelPos;
    uint2 texDim;
    int kernelRadius = 1;

    int x = pixelPos.x;
    int y = pixelPos.y;
    int height = texDim.y;
    int width = texDim.x;


    float weight = 0.0;
    
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
                filteredImage(i_x, i_y) = frameInfo.m_beauty(i_x, i_y);
                filteredNormal(i_x, i_y) =
						Normalize(frameInfo.m_normal(i_x, i_y));
                filteredPos(i_x, i_y) = frameInfo.m_position(i_x, i_y);
                float DisSqr = distanceSqr(x, y, i_x, i_y);
                float colorDistSqr =
						SqrLength(filteredImage(x, y) - filteredImage(i_x, i_y));
                float NormDot =
						Dot(filteredNormal(x, y), filteredNormal(i_x, i_y));
                NormDot = std::clamp(NormDot, 0.0f, 1.0f);

                float DnormalSqr = Sqr(SafeAcos(NormDot));
                Float3 DistancePos =
						Normalize((filteredPos(i_x, i_y) - filteredPos(x, y)));
                float Dplane = Dot(filteredNormal(x, y), DistancePos);
                Dplane = std::clamp(Dplane, 1e-5f, 1.0f);

                w_ixy = J_kernel(DisSqr, colorDistSqr, DnormalSqr, Dplane,
									 m_sigmaCoord, m_sigmaColor, m_sigmaNormal,
									 m_sigmaPlane);
                FinalImage(x, y) += filteredImage(i_x, i_y) * w_ixy;
					// add weights
                weight += w_ixy;
            }
        }
    }
    if (0.0 != weight)
    {
        FinalImage(x, y) /= weight;
    }
    else
    {
        FinalImage(x, y) = 0.0;
    }

    filteredImage(x, y) = float(0.0);
    
}
