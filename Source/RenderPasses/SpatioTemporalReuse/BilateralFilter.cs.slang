#include "BilateralFilterConstants.slangh"
#include "BilateralFilterCommon.slang"

RWTexture2D<float4> gTexOutput;

[numthreads(_BILATERAL_THREAD_NUM_X, _BILATERAL_THREAD_NUM_Y, 1)]
void main(int3 groupID : SV_GroupID, int3 threadID : SV_GroupThreadID)
{
    uint3 Dim;
    gTexColor.GetDimensions(0, Dim.x, Dim.y, Dim.z);
    
    int2 PixelPos = int2(groupID.x * _BILATERAL_THREAD_NUM_X + threadID.x + 0.5, groupID.y * _BILATERAL_THREAD_NUM_Y + threadID.y + 0.5);
    gTexOutput[PixelPos] = float4(separateJBFPassKernel(PixelPos, Dim.xy), 1.0);
}
