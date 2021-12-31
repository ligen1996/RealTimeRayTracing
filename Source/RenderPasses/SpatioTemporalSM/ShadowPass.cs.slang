#include "ShadowMapConstant.slangh"

StructuredBuffer<float3> gPointList;
RWTexture2DArray<uint> gOutputShadowMap;
RWTexture2D<float3> gOutputDebug;
// interlock/atomic operation only support int/uint, so depth need to store as int/uint

cbuffer PerFrameCB : register(b0)
{
    SShadowMapData gShadowMapData;
    uint gNumPoint;
}

int getPointNum()
{
    //return (int)((uint)(gNumPoint.Load(0)));
    return (int)gNumPoint;
}

void fillMapWithPoint(float vPointIndex, int vMapIndex, uint2 vMapSize)
{
    float3 Point = gPointList[vPointIndex];
    float4x4 VP = gShadowMapData.allGlobalMat[vMapIndex];
    float4 PosH = mul(float4(Point, 1.0f), VP);
#ifdef FALCOR_D3D
    PosH.y = -PosH.y;
#endif
    float3 UVW = (PosH.xyz / PosH.w) * 0.5 + 0.5;
    uint3 TargetIndex = uint3(round(UVW.x * vMapSize.x + 0.5), round(UVW.y * vMapSize.y + 0.5), vMapIndex);
    //uint3 TargetIndex = uint3(UVW.x * vMapSize.x, UVW.y * vMapSize.y, vMapIndex);
    //uint Depth = asuint(UVW.z); // FIXME: directly convert to uint? is the comparasion correct?
    uint Depth = uint(UVW.z * 0xffffffffu);

    if (gOutputShadowMap[TargetIndex] == 0)
        gOutputShadowMap[TargetIndex] = Depth; 
    else
        InterlockedMin(gOutputShadowMap[TargetIndex], Depth);
    //gOutputShadowMap[TargetIndex] = vPointIndex;
    gOutputDebug[TargetIndex.xy] = Point;
}

// x and y has strict limitation, so multipass is needed
[numthreads(_SHADOW_MAP_SHADER_THREAD_NUM_X, _SHADOW_MAP_SHADER_THREAD_NUM_Y, _SHADOW_MAP_SHADER_THREAD_NUM_Z)]
void main(int3 groupID : SV_GroupID, int3 threadID : SV_GroupThreadID)
{
    int PointNum = getPointNum();
    int StartPointIndex = (groupID.x * _SHADOW_MAP_SHADER_THREAD_NUM_X + threadID.x) * _SHADOW_MAP_SHADER_POINT_PER_THREAD;
    int EndPointIndex = min(StartPointIndex + _SHADOW_MAP_SHADER_POINT_PER_THREAD, PointNum);

    uint3 Dimension;
    gOutputShadowMap.GetDimensions(Dimension.x, Dimension.y, Dimension.z);
    int MapNum = (int)Dimension.z;
    int StartMapIndex = (groupID.y * _SHADOW_MAP_SHADER_THREAD_NUM_Y + threadID.y) * _SHADOW_MAP_SHADER_MAP_PER_THREAD;
    int EndMapIndex = min(StartMapIndex + _SHADOW_MAP_SHADER_MAP_PER_THREAD, MapNum);
    
    //[unroll(_SHADOW_MAP_SHADER_POINT_PER_THREAD)]
    for (int PointIndex = StartPointIndex; PointIndex < EndPointIndex; ++PointIndex)
    {
        //[unroll(_SHADOW_MAP_SHADER_MAP_PER_THREAD)]
        for (int MapIndex = StartMapIndex; MapIndex < EndMapIndex; ++MapIndex)
        {
            fillMapWithPoint(PointIndex, MapIndex, Dimension.xy);
        }
    }
}
