#pragma once
void clipPolygonalLightByZPlane(inout float3 vioPolygonalLightVertexPos[5], out int voPolygonalLightVertexNumAfterClipping)
{
    int Flag4HowManyVertexAboveZPlane = 0;
    if (vioPolygonalLightVertexPos[0].z > 0.0)
        Flag4HowManyVertexAboveZPlane += 1;
    if (vioPolygonalLightVertexPos[1].z > 0.0)
        Flag4HowManyVertexAboveZPlane += 2;
    if (vioPolygonalLightVertexPos[2].z > 0.0)
        Flag4HowManyVertexAboveZPlane += 4;
    if (vioPolygonalLightVertexPos[3].z > 0.0)
        Flag4HowManyVertexAboveZPlane += 8;

    voPolygonalLightVertexNumAfterClipping = 0;
    if (Flag4HowManyVertexAboveZPlane == 0)
    {
    }
    else if (Flag4HowManyVertexAboveZPlane == 1)
    {
        voPolygonalLightVertexNumAfterClipping = 3;
        vioPolygonalLightVertexPos[1] = -vioPolygonalLightVertexPos[1].z * vioPolygonalLightVertexPos[0] + vioPolygonalLightVertexPos[0].z * vioPolygonalLightVertexPos[1];
        vioPolygonalLightVertexPos[2] = -vioPolygonalLightVertexPos[3].z * vioPolygonalLightVertexPos[0] + vioPolygonalLightVertexPos[0].z * vioPolygonalLightVertexPos[3];
    }
    else if (Flag4HowManyVertexAboveZPlane == 2)
    {
        voPolygonalLightVertexNumAfterClipping = 3;
        vioPolygonalLightVertexPos[0] = -vioPolygonalLightVertexPos[0].z * vioPolygonalLightVertexPos[1] + vioPolygonalLightVertexPos[1].z * vioPolygonalLightVertexPos[0];
        vioPolygonalLightVertexPos[2] = -vioPolygonalLightVertexPos[2].z * vioPolygonalLightVertexPos[1] + vioPolygonalLightVertexPos[1].z * vioPolygonalLightVertexPos[2];
    }
    else if (Flag4HowManyVertexAboveZPlane == 3)
    {
        voPolygonalLightVertexNumAfterClipping = 4;
        vioPolygonalLightVertexPos[2] = -vioPolygonalLightVertexPos[2].z * vioPolygonalLightVertexPos[1] + vioPolygonalLightVertexPos[1].z * vioPolygonalLightVertexPos[2];
        vioPolygonalLightVertexPos[3] = -vioPolygonalLightVertexPos[3].z * vioPolygonalLightVertexPos[0] + vioPolygonalLightVertexPos[0].z * vioPolygonalLightVertexPos[3];
    }
    else if (Flag4HowManyVertexAboveZPlane == 4)
    {
        voPolygonalLightVertexNumAfterClipping = 3;
        vioPolygonalLightVertexPos[0] = -vioPolygonalLightVertexPos[3].z * vioPolygonalLightVertexPos[2] + vioPolygonalLightVertexPos[2].z * vioPolygonalLightVertexPos[3];
        vioPolygonalLightVertexPos[1] = -vioPolygonalLightVertexPos[1].z * vioPolygonalLightVertexPos[2] + vioPolygonalLightVertexPos[2].z * vioPolygonalLightVertexPos[1];
    }
    else if (Flag4HowManyVertexAboveZPlane == 5)
    {
        voPolygonalLightVertexNumAfterClipping = 0;
    }
    else if (Flag4HowManyVertexAboveZPlane == 6)
    {
        voPolygonalLightVertexNumAfterClipping = 4;
        vioPolygonalLightVertexPos[0] = -vioPolygonalLightVertexPos[0].z * vioPolygonalLightVertexPos[1] + vioPolygonalLightVertexPos[1].z * vioPolygonalLightVertexPos[0];
        vioPolygonalLightVertexPos[3] = -vioPolygonalLightVertexPos[3].z * vioPolygonalLightVertexPos[2] + vioPolygonalLightVertexPos[2].z * vioPolygonalLightVertexPos[3];
    }
    else if (Flag4HowManyVertexAboveZPlane == 7)
    {
        voPolygonalLightVertexNumAfterClipping = 5;
        vioPolygonalLightVertexPos[4] = -vioPolygonalLightVertexPos[3].z * vioPolygonalLightVertexPos[0] + vioPolygonalLightVertexPos[0].z * vioPolygonalLightVertexPos[3];
        vioPolygonalLightVertexPos[3] = -vioPolygonalLightVertexPos[3].z * vioPolygonalLightVertexPos[2] + vioPolygonalLightVertexPos[2].z * vioPolygonalLightVertexPos[3];
    }
    else if (Flag4HowManyVertexAboveZPlane == 8)
    {
        voPolygonalLightVertexNumAfterClipping = 3;
        vioPolygonalLightVertexPos[0] = -vioPolygonalLightVertexPos[0].z * vioPolygonalLightVertexPos[3] + vioPolygonalLightVertexPos[3].z * vioPolygonalLightVertexPos[0];
        vioPolygonalLightVertexPos[1] = -vioPolygonalLightVertexPos[2].z * vioPolygonalLightVertexPos[3] + vioPolygonalLightVertexPos[3].z * vioPolygonalLightVertexPos[2];
        vioPolygonalLightVertexPos[2] = vioPolygonalLightVertexPos[3];
    }
    else if (Flag4HowManyVertexAboveZPlane == 9)
    {
        voPolygonalLightVertexNumAfterClipping = 4;
        vioPolygonalLightVertexPos[1] = -vioPolygonalLightVertexPos[1].z * vioPolygonalLightVertexPos[0] + vioPolygonalLightVertexPos[0].z * vioPolygonalLightVertexPos[1];
        vioPolygonalLightVertexPos[2] = -vioPolygonalLightVertexPos[2].z * vioPolygonalLightVertexPos[3] + vioPolygonalLightVertexPos[3].z * vioPolygonalLightVertexPos[2];
    }
    else if (Flag4HowManyVertexAboveZPlane == 10)
    {
        voPolygonalLightVertexNumAfterClipping = 0;
    }
    else if (Flag4HowManyVertexAboveZPlane == 11)
    {
        voPolygonalLightVertexNumAfterClipping = 5;
        vioPolygonalLightVertexPos[4] = vioPolygonalLightVertexPos[3];
        vioPolygonalLightVertexPos[3] = -vioPolygonalLightVertexPos[2].z * vioPolygonalLightVertexPos[3] + vioPolygonalLightVertexPos[3].z * vioPolygonalLightVertexPos[2];
        vioPolygonalLightVertexPos[2] = -vioPolygonalLightVertexPos[2].z * vioPolygonalLightVertexPos[1] + vioPolygonalLightVertexPos[1].z * vioPolygonalLightVertexPos[2];
    }
    else if (Flag4HowManyVertexAboveZPlane == 12)
    {
        voPolygonalLightVertexNumAfterClipping = 4;
        vioPolygonalLightVertexPos[1] = -vioPolygonalLightVertexPos[1].z * vioPolygonalLightVertexPos[2] + vioPolygonalLightVertexPos[2].z * vioPolygonalLightVertexPos[1];
        vioPolygonalLightVertexPos[0] = -vioPolygonalLightVertexPos[0].z * vioPolygonalLightVertexPos[3] + vioPolygonalLightVertexPos[3].z * vioPolygonalLightVertexPos[0];
    }
    else if (Flag4HowManyVertexAboveZPlane == 13)
    {
        voPolygonalLightVertexNumAfterClipping = 5;
        vioPolygonalLightVertexPos[4] = vioPolygonalLightVertexPos[3];
        vioPolygonalLightVertexPos[3] = vioPolygonalLightVertexPos[2];
        vioPolygonalLightVertexPos[2] = -vioPolygonalLightVertexPos[1].z * vioPolygonalLightVertexPos[2] + vioPolygonalLightVertexPos[2].z * vioPolygonalLightVertexPos[1];
        vioPolygonalLightVertexPos[1] = -vioPolygonalLightVertexPos[1].z * vioPolygonalLightVertexPos[0] + vioPolygonalLightVertexPos[0].z * vioPolygonalLightVertexPos[1];
    }
    else if (Flag4HowManyVertexAboveZPlane == 14)
    {
        voPolygonalLightVertexNumAfterClipping = 5;
        vioPolygonalLightVertexPos[4] = -vioPolygonalLightVertexPos[0].z * vioPolygonalLightVertexPos[3] + vioPolygonalLightVertexPos[3].z * vioPolygonalLightVertexPos[0];
        vioPolygonalLightVertexPos[0] = -vioPolygonalLightVertexPos[0].z * vioPolygonalLightVertexPos[1] + vioPolygonalLightVertexPos[1].z * vioPolygonalLightVertexPos[0];
    }
    else if (Flag4HowManyVertexAboveZPlane == 15)
    {
        voPolygonalLightVertexNumAfterClipping = 4;
    }

    if (voPolygonalLightVertexNumAfterClipping == 3)
        vioPolygonalLightVertexPos[3] = vioPolygonalLightVertexPos[0];
    if (voPolygonalLightVertexNumAfterClipping == 4)
        vioPolygonalLightVertexPos[4] = vioPolygonalLightVertexPos[0];
}

float3 integrateEdge(float3 vVertex1, float3 vVertex2)
{
    float CosTheta = dot(vVertex1, vVertex2);
    float Theta = acos(CosTheta);
    return cross(vVertex1, vVertex2) * ((Theta > 0.001) ? Theta / sin(Theta) : 1.0);
}

struct SRay
{
    float3 m_Origin;
    float3 m_Dir;
};

bool rayPlaneIntersect(SRay vRay, float4 vPlane, out float voDistanceFromRayOrigin2Plane)
{
    voDistanceFromRayOrigin2Plane = -dot(vPlane, float4(vRay.m_Origin, 1.0)) / dot(vPlane.xyz, vRay.m_Dir);
    return voDistanceFromRayOrigin2Plane > 0.0;
}

float3 getPosT(float3 vNormal, float3 vViewDir, float3 vFragPos, float3 VecW)
{
    float3 Tangent = normalize(vViewDir - vNormal * dot(vViewDir, vNormal));
    float3 Bitangent = cross(vNormal, Tangent);

    return mul(VecW, transpose(float3x3(Tangent, Bitangent, vNormal)));
}
