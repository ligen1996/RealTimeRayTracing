import Scene.Raster;

cbuffer PerLightCB : register(b0)
{
    float4x4 gGlobalMat;
}

float4 vsMain(VSIn vIn) : SV_POSITION
{
    const GeometryInstanceID instanceID = { vIn.meshInstanceID };
    float4x4 worldMat = gScene.getWorldMatrix(instanceID);
    float3 posW = mul(float4(vIn.pos, 1.f), worldMat).xyz;
    return mul(float4(posW, 1.f), gGlobalMat);
}

float psMain(float4 vPos : SV_POSITION) : SV_Target0
{
    return saturate(vPos.z * 0.5 + 0.5);
}
