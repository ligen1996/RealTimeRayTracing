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
    //Result.posH = mul(float4(vIn.pos, 1.0f), gScene.camera.getViewProj());
    return Result;
}

float4 psMain(VSOutCustom vOut) : SV_Target0
{
    //PointList.Append(vOut.posW);
    //return float4(vOut.posW, 1.0f);
    return float4(vOut.posW, 1.0f);
}
