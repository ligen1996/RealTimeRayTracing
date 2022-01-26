import Scene.Raster;

AppendStructuredBuffer<float4> PointAndIdList;

cbuffer PerFrameCB : register(b0)
{
    float4x4 gViewProjectMat;
};

struct VSOutCustom
{
    linear float3 posW : POSW; ///< Position in world space.
    float4 posH : SV_POSITION;
    uint ID : DRAW_ID;
};

VSOutCustom vsMain(VSIn vIn)
{
    const GeometryInstanceID InstanceID = { vIn.meshInstanceID };
    float4x4 ModelMat = gScene.getWorldMatrix(InstanceID);
    float3 PosW = mul(float4(vIn.pos, 1.f), ModelMat).xyz;
    
    VSOutCustom Result;
    Result.posW = PosW;
    Result.posH = mul(float4(PosW, 1.0f), gViewProjectMat);
    Result.ID = vIn.meshInstanceID;
    return Result;
}

void psMain(VSOutCustom vOut)
{
    PointAndIdList.Append(float4(vOut.posW, asfloat(vOut.ID)));
}
