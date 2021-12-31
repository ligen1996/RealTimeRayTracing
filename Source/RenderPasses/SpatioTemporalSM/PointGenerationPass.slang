import Scene.Raster;

AppendStructuredBuffer<float3> PointList;

cbuffer PerFrameCB : register(b0)
{
    float4x4 gViewProjectMat;
};

struct VSOutCustom
{
    linear float3 posW : POSW; ///< Position in world space.
    float4 posH : SV_POSITION;
};

VSOutCustom vsMain(VSIn vIn)
{
    VSOutCustom Result;
    Result.posW = vIn.pos;
    Result.posH = mul(float4(vIn.pos, 1.0f), gViewProjectMat);
    return Result;
}

void psMain(VSOutCustom vOut)
{
    PointList.Append(vOut.posW);
}
