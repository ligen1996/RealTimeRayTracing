//#include "..\LTCLight\LTCData.slangh"
//#include "..\LTCLight\LTCMath.slang"

//Texture2D gPosW;
//Texture2D gNormalW;
//Texture2D gLightColor;
//Texture2D gLTCMatrix;
//Texture2D gLTCMagnitue;

//cbuffer PerFrameCB
//{
//    SPassData g;
//};

//uniform float LUT_SIZE = 64.0;
//uniform float LUT_SCALE = (LUT_SIZE - 1.0) / LUT_SIZE;
//uniform float LUT_BIAS = 0.5 / LUT_SIZE;
//uniform float PI = 3.14159265;

//float3 fecthFilteredLightTexture(float3 vPolygonalLightVertexPos[4], float3 vLooupfloattor)
//{
//    float3 V1 = vPolygonalLightVertexPos[1] - vPolygonalLightVertexPos[0];
//    float3 V2 = vPolygonalLightVertexPos[3] - vPolygonalLightVertexPos[0];
//    float3 PlaneOrtho = cross(V1, V2);
//    float PlaneAreaSquared = dot(PlaneOrtho, PlaneOrtho);
	
//    SRay Ray;
//    Ray.m_Origin = float3(0);
//    Ray.m_Dir = vLooupfloattor;
//    float4 Plane = float4(PlaneOrtho, -dot(PlaneOrtho, vPolygonalLightVertexPos[0]));
//    float Distance2Plane;
//    rayPlaneIntersect(Ray, Plane, Distance2Plane);
	
//    float3 P = Distance2Plane * Ray.m_Dir - vPolygonalLightVertexPos[0];

//    float Dot_V1_V2 = dot(V1, V2);
//    float Inv_Dot_V1_V1 = 1.0 / dot(V1, V1);
//    float3 V2_ = V2 - V1 * Dot_V1_V2 * Inv_Dot_V1_V1;
//    float2 UV;
//    UV.y = dot(V2_, P) / dot(V2_, V2_);
//    UV.x = dot(V1, P) * Inv_Dot_V1_V1 - Dot_V1_V2 * Inv_Dot_V1_V1 * UV.y;
//    UV = float2(1 - UV.y, 1 - UV.x);
	
//    float Distance = abs(Distance2Plane) / pow(PlaneAreaSquared, 0.25);
	
//    float Lod = log(2048.0 * Distance) / log(3.0);

//    float LodA = floor(Lod);
//    float LodB = ceil(Lod);
//    float t = Lod - LodA;
//    //float3 ColorA = texture(gLightColor, float3(UV, LodA)).rgb;
//    //float3 ColorB = texture(gLightColor, float3(UV, LodB)).rgb;
//    //return lerp(ColorA, ColorB, t);
//    return float3(1);
//}


//float3 integrateLTC(float3 vNormal, float3 vViewDir, float3 vFragPos, float3x3 vLTCMatrix, float3 vPolygonalLightVertexPos[4], bool vTwoSided)
//{
//    float3 Tangent = normalize(vViewDir - vNormal * dot(vViewDir, vNormal));
//    float3 Bitangent = cross(vNormal, Tangent);


//    vLTCMatrix = vLTCMatrix * transpose(float3x3(Tangent, Bitangent, vNormal));

//    float3 PolygonalLightVertexPosInTangentSpace[5], PolygonalLightVertexPosBeforeClipping[4];
//    PolygonalLightVertexPosBeforeClipping[0] = PolygonalLightVertexPosInTangentSpace[0] = mul((vPolygonalLightVertexPos[0] - vFragPos), vLTCMatrix);
//    PolygonalLightVertexPosBeforeClipping[1] = PolygonalLightVertexPosInTangentSpace[1] = mul((vPolygonalLightVertexPos[1] - vFragPos), vLTCMatrix);
//    PolygonalLightVertexPosBeforeClipping[2] = PolygonalLightVertexPosInTangentSpace[2] = mul((vPolygonalLightVertexPos[2] - vFragPos), vLTCMatrix);
//    PolygonalLightVertexPosBeforeClipping[3] = PolygonalLightVertexPosInTangentSpace[3] = mul((vPolygonalLightVertexPos[3] - vFragPos), vLTCMatrix);
//    PolygonalLightVertexPosInTangentSpace[4] = PolygonalLightVertexPosInTangentSpace[3];

//    int PolygonalLightVertexNumAfterClipping;
//    clipPolygonalLightByZPlane(PolygonalLightVertexPosInTangentSpace, PolygonalLightVertexNumAfterClipping);
	
//    if (PolygonalLightVertexNumAfterClipping == 0)
//        return float3(0);
	
//    PolygonalLightVertexPosInTangentSpace[0] = normalize(PolygonalLightVertexPosInTangentSpace[0]);
//    PolygonalLightVertexPosInTangentSpace[1] = normalize(PolygonalLightVertexPosInTangentSpace[1]);
//    PolygonalLightVertexPosInTangentSpace[2] = normalize(PolygonalLightVertexPosInTangentSpace[2]);
//    PolygonalLightVertexPosInTangentSpace[3] = normalize(PolygonalLightVertexPosInTangentSpace[3]);
//    PolygonalLightVertexPosInTangentSpace[4] = normalize(PolygonalLightVertexPosInTangentSpace[4]);

//    float3 VSum = float3(0.0);
//    VSum += integrateEdge(PolygonalLightVertexPosInTangentSpace[0], PolygonalLightVertexPosInTangentSpace[1]);
//    VSum += integrateEdge(PolygonalLightVertexPosInTangentSpace[1], PolygonalLightVertexPosInTangentSpace[2]);
//    VSum += integrateEdge(PolygonalLightVertexPosInTangentSpace[2], PolygonalLightVertexPosInTangentSpace[3]);
//    if (PolygonalLightVertexNumAfterClipping >= 4)
//        VSum += integrateEdge(PolygonalLightVertexPosInTangentSpace[3], PolygonalLightVertexPosInTangentSpace[4]);
//    if (PolygonalLightVertexNumAfterClipping == 5)
//        VSum += integrateEdge(PolygonalLightVertexPosInTangentSpace[4], PolygonalLightVertexPosInTangentSpace[0]);

//    float Sum = vTwoSided ? abs(VSum.z) : max(VSum.z, 0.0);

//    float3 Lookupfloattor = normalize(VSum);
//    float3 TextureLight = fecthFilteredLightTexture(PolygonalLightVertexPosBeforeClipping, Lookupfloattor);

//    return float3(Sum) * TextureLight;
//}

//float2 LTC_Coords(float vCosTheta, float vRoughness)
//{
//    uint2 texDim;
//    uint levels;
//    gLTCMatrix.GetDimensions(0, texDim.x, texDim.y, levels);

//    float Theta = acos(vCosTheta);
//    float2 Coords = float2(vRoughness, Theta / (0.5 * PI));
//    const float LUT_SIZE = 32.0;
//    // scale and bias coordinates, for correct filtered lookup
//    Coords = Coords * (LUT_SIZE - 1.0) / LUT_SIZE + 0.5 / LUT_SIZE;

//    return Coords * texDim;
//}

float4 main(float2 texC : TEXCOORD) : SV_Target0
{
 //   uint2 texDim;
 //   uint levels;
 //   gPosW.GetDimensions(0, texDim.x, texDim.y, levels);

 //   float2 pixelPos = texC * texDim;
 //   int2 iPos = int2(pixelPos);

 //   float3 PosW = gPosW[iPos].xyz;
 //   float3 GroundNormal = gNormalW[iPos].xyz; //其他几何体的话应该由其法线乘以模型矩阵来算
 //   float3 ViewDir = normalize(g.CamPosW.xyz - PosW);

 //   float2 LTXCrd = LTC_Coords(dot(GroundNormal, ViewDir), g.Roughness);

 //   float4 LTCMatrixComponents = gLTCMatrix[LTXCrd];
 //   float3x3 LTCMatrix = float3x3
	//(
	//	float3(1, 0, LTCMatrixComponents.y),
	//	float3(0, LTCMatrixComponents.z, 0),
	//	float3(LTCMatrixComponents.w, 0, LTCMatrixComponents.x)
	//);
 //   LTCMatrix = transpose(LTCMatrix);

 //   float3 Diffuse = integrateLTC(GroundNormal, ViewDir, PosW, float3x3(1, 0, 0, 0, 1, 0, 0, 0, 1), g.LightPolygonPoints, g.TwoSide > 0.5);
 //   float3 Specular = integrateLTC(GroundNormal, ViewDir, PosW, LTCMatrix, g.LightPolygonPoints, g.TwoSide > 0.5);
 //   Specular *= gLTCMagnitue[LTXCrd].w;
 //   float2 Schlick = gLTCMagnitue[LTXCrd].xy;
	//Specular *= g.SpecularColor.xyz * Schlick.x + (1.0 - g.SpecularColor.xyz) * Schlick.y;

 //   float3 ResultColor = g.Intensity * (Diffuse * g.DiffuseColor.xyz + Specular / 10);
 //   ResultColor /= PI;
    //return float4(ResultColor, 1.0);
    return float4(0,1,0, 1.0);
}
