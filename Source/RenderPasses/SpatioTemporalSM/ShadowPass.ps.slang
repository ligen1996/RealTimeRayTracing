import Scene.Raster;

cbuffer PerLightCB : register(b0)
{
    float4x4 gGlobalMat;
}

struct SVSOut
{
    uint ID : DRAW_ID;
    float4 PosH : SV_POSITION;
};

SVSOut vsMain(VSIn vIn) : SV_POSITION
{
    const GeometryInstanceID instanceID = { vIn.meshInstanceID };
    float4x4 worldMat = gScene.getWorldMatrix(instanceID);
    float3 posW = mul(float4(vIn.pos, 1.f), worldMat).xyz;

    SVSOut Out;
    Out.PosH = mul(float4(posW, 1.f), gGlobalMat);
    Out.ID = vIn.meshInstanceID;
    return Out;
}

struct SPsOut
{
    float Depth : SV_Target0;
    uint ID : SV_Target1;
};

SPsOut psMain(SVSOut vIn) : SV_Target0
{
    SPsOut Out;
    Out.Depth = saturate(vIn.PosH.z * 0.5 + 0.5);
    Out.ID = vIn.ID;
    return Out;
}
