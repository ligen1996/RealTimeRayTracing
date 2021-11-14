/***************************************************************************
 # Copyright (c) 2015-21, NVIDIA CORPORATION. All rights reserved.
 #
 # Redistribution and use in source and binary forms, with or without
 # modification, are permitted provided that the following conditions
 # are met:
 #  * Redistributions of source code must retain the above copyright
 #    notice, this list of conditions and the following disclaimer.
 #  * Redistributions in binary form must reproduce the above copyright
 #    notice, this list of conditions and the following disclaimer in the
 #    documentation and/or other materials provided with the distribution.
 #  * Neither the name of NVIDIA CORPORATION nor the names of its
 #    contributors may be used to endorse or promote products derived
 #    from this software without specific prior written permission.
 #
 # THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS "AS IS" AND ANY
 # EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 # IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 # PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 # CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 # EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 # PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 # PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 # OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 # (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 # OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **************************************************************************/

/** Minimal path tracer.

    The purpose is to use it for validation of more complex renderers.
    The implementation here should be kept as simple/naive as possible.

    At each hit point (including the primary hit loaded from the G-buffer),
    analytic light sources (point, directional) are sampled uniformly using
    1 shadow ray, and 1 scatter ray is traced to sample the hemisphere.
    At hit/miss the scatter ray includes light from emissive surface and
    the environment map, respectively. Traversal stops at a fixed path length.

    Each type of light (analytic, emissive, env map) can be individually
    enabled/disabled from the host. This clutters the code a bit, but it is
    important as not all other renderes may support all three light types.

    The host sets the following defines:

    MAX_BOUNCES         Maximum number of indirect bounces (0 means no indirect).
    COMPUTE_DIRECT      Nonzero if direct illumination should be included.
    USE_ANALYTIC_LIGHTS Nonzero if Falcor's analytic lights should be used.
    USE_EMISSIVE_LIGHTS Nonzero if emissive geometry should be used as lights.
    USE_ENV_LIGHT       Nonzero if env map is available and should be used as light source.
    USE_ENV_BACKGROUND  Nonzero if env map is available and should be used as background.
    is_valid_<name>     1 if optional I/O buffer with this name should be used.
*/

#include "Utils/Math/MathConstants.slangh"

import Scene.Raytracing;
import Scene.HitInfo;//lg
import Utils.Math.MathHelpers;
import Utils.Sampling.SampleGenerator;
import Experimental.Scene.Material.StandardMaterial;
import Experimental.Scene.Material.MaterialHelpers;
import Experimental.Scene.Lights.LightHelpers;
import Utils.Geometry.IntersectionHelpers; //ray intersect with plane //lg

cbuffer CB
{
    uint        gFrameCount;        // Frame count since scene was loaded.
    uint        gPRNGDimension;     // First available PRNG dimension.
}

static struct Data
{
    // Materials
    StandardMaterial standardMaterial;
} gData;

// Inputs
Texture2D<float4> gWorldPosition;
Texture2D<float4> gWorldShadingNormal;
Texture2D<float4> gWorldShadingTangent; // Optional
Texture2D<float4> gWorldFaceNormal;
Texture2D<float4> gWorldView; // Optional
Texture2D<float4> gMaterialDiffuseOpacity;
Texture2D<float4> gMaterialSpecularRoughness;
Texture2D<float4> gMaterialEmissive;
Texture2D<float4> gMaterialExtraParams;
Texture2D<PackedHitInfo> gVBuffer;

// Outputs
RWTexture2D<float4> gOutputColor;
RWTexture2D<float>  gOutputVisibility;
RWTexture2D<float4> gShadowMotionVector; //lg :;float3 can error ,float2 and float4 
RWTexture2D<float4> gTest;//lg Test Last Fram

// Static configuration based on defines set from the host.
#define is_valid(name) (is_valid_##name != 0)
static const uint kMaxBounces = MAX_BOUNCES;
static const bool kComputeDirect = COMPUTE_DIRECT;
static const bool kUseAnalyticLights = USE_ANALYTIC_LIGHTS;
static const bool kUseEmissiveLights = USE_EMISSIVE_LIGHTS;
static const bool kUseEnvLight = USE_ENV_LIGHT;
static const bool kUseEnvBackground = USE_ENV_BACKGROUND;
static const float3 kDefaultBackgroundColor = float3(0, 0, 0);
static const float kRayTMax = FLT_MAX;

/** Payload for shadow ray.
*/
struct ShadowRayData
{
    bool visible;

    PackedHitInfo packedHitInfo;
};

/** Payload for scatter ray (80B).
*/
struct ScatterRayData
{
    float3  radiance;       ///< Accumulated outgoing radiance from path.
    bool    terminated;     ///< Set to true when path is terminated.
    float3  thp;            ///< Current path throughput. This is updated at each path vertex.
    uint    pathLength;     ///< Path length in number of path segments (0 at origin, 1 at first secondary hit, etc.). Max 2^31.
    float3  origin;         ///< Next path segment origin.
    uint    _pad0;
    float3  direction;      ///< Next path segment direction.
    uint    _pad1;

    SampleGenerator sg;     ///< Per-ray state for the sample generator (up to 16B).

    /** Create ray payload with default parameters.
    */
    static ScatterRayData create(SampleGenerator sg)
    {
        ScatterRayData d;
        d.terminated = false;
        d.pathLength = 0;
        d.radiance = float3(0, 0, 0);
        d.thp = float3(1, 1, 1);
        d.origin = float3(0, 0, 0);
        d.direction = float3(0, 0, 0);
        d.sg = sg;
        return d;
    }
};

/** Helper to load the material attributes.
*/
MaterialParams loadMaterialParams(uint2 pixelPos)
{
    MaterialParams matParams;
    matParams.diffuseOpacity = gMaterialDiffuseOpacity[pixelPos];
    matParams.specularRoughness = gMaterialSpecularRoughness[pixelPos];
    matParams.emissive = gMaterialEmissive[pixelPos];
    matParams.extraParams = gMaterialExtraParams[pixelPos];

    return matParams;
}

/** Returns the primary ray's direction.
*/
float3 getPrimaryRayDir(uint2 launchIndex, uint2 launchDim, const Camera camera)
{
    if (is_valid(gWorldView))
    {
        // If we have the view vector bound as a buffer, just fetch it. No need to compute anything.
        return -gWorldView[launchIndex].xyz;
    }
    else
    {
        // Compute the view vector. This must exactly match what the G-buffer pass is doing (jitter etc.).
        // Note that we do not take depth-of-field into account as it would require exactly matching the
        // sample generator between the passes, which is error prone. The host side will issue a warning instead.
        return camera.computeRayPinhole(launchIndex, launchDim).dir;
    }
}

/** Traces a shadow ray towards a light source.
    \param[in] origin Ray origin for the shadow ray.
    \param[in] dir Direction from shading point towards the light source (normalized).
    \param[in] distance Distance to the light source.
    \return True if light is visible, false otherwise.
*/
bool traceShadowRay(float3 origin, float3 dir, float distance, out PackedHitInfo packedHiInfo) //lg
{
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = dir;
    ray.TMin = 0.f;
    ray.TMax = distance;

    ShadowRayData rayData;
    rayData.visible = false;    // Set to true by miss shader if ray is not terminated before
    TraceRay(gScene.rtAccel, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xff /* instanceInclusionMask */, 1 /* hitIdx */, rayTypeCount, 1 /* missIdx */, ray, rayData);
    packedHiInfo = rayData.packedHitInfo;
    return rayData.visible;
}

/** Traces a scatter ray based on ray parameters stored in the ray payload.
    \param[in] rayData Describes the ray parameters. The struct is modified based on the result.
*/
void traceScatterRay(inout ScatterRayData rayData)
{
    RayDesc ray;
    ray.Origin = rayData.origin;
    ray.Direction = rayData.direction;
    ray.TMin = 0.f;
    ray.TMax = kRayTMax;

    uint rayFlags = 0;      // TODO: Set cull mode from the app
    TraceRay(gScene.rtAccel, rayFlags, 0xff /* instanceInclusionMask */, 0 /* hitIdx */, rayTypeCount, 0 /* missIdx */, ray, rayData);
}

/** Evaluates the direct illumination from analytic lights.
    This function samples Falcor's light list uniformly with one shadow ray.
    \param[in] sd Shading data.
    \param[in] rayOrigin Ray origin for the shadow ray.
    \param[in] sg SampleGenerator object.
    \return Outgoing radiance in view direction.
*/
float3 evalDirectAnalytic(const ShadingData sd, float3 rayOrigin, inout SampleGenerator sg) //lg
{
    const uint lightCount = gScene.getLightCount();
    if (lightCount == 0) return float3(0);

    // Pick one of the analytic light sources randomly with equal probability.
    const uint lightIndex = min(uint(sampleNext1D(sg) * lightCount), lightCount - 1);
    float invPdf = lightCount; // Light selection pdf = 1.0 / lightCount. 

    // Sample local light source.
    AnalyticLightSample ls;
    bool valid = sampleLight(rayOrigin, gScene.getLight(lightIndex), sg, ls);

    // Reject sample if lower hemisphere.
    if (!valid || dot(ls.dir, sd.N) <= kMinCosTheta) return float3(0);
    // Test visibility by tracing a shadow ray.
    
    PackedHitInfo packedShadowHitInfo;
    bool V = traceShadowRay(rayOrigin, ls.dir, ls.distance, packedShadowHitInfo);
    return V ? gData.standardMaterial.eval(sd, ls.dir) * ls.Li * invPdf : float3(0);
}

//lg add
//calculate ray intersection with plane
float4 rayPlaneIntersect(float3 origin,float3 dir,float3 Pos,float3 Normal)
{
    float ResultT = 0.0f;
    float4 Intersection = float4(0.f, 0.f, 0.f, 0.f);
    if (intersectRayPlane(origin, dir, Pos, Normal, ResultT))
    {
        Intersection.xyz = origin + ResultT * dir;
        Intersection.w = 1.0f;
    }

    //Intersection.w = ResultT;//lgTest
    
    return Intersection;
}


//lg
//calculate shadow motion vector in falcor way
//primay hit for hitPos,hitNormal,hitMeshID
//shadow hit for blockerPos,blockerMeshId
//ligth Pos, and prevLightPos
//light not move for this test
float2 calcShdowMotionVectorInternel( uint2 PixelIndex, const TriangleHit PrimaryHit, float3 Normal,
const TriangleHit ShadowHit, float3 LightPos, float2 RenderTargetDim)
{
    float2 X = PixelIndex + float2(0.5f, 0.5f);
    
    float3 prevS = gScene.getPrevPosW(PrimaryHit);
    float3 prevB = gScene.getPrevPosW(ShadowHit);
    float3 prevL = LightPos; //todo light

    //gOutputColor[PixelIndex] = float4(prevS,1.0f);
    
    //prevNormal
    float3x3 invTransMat = gScene.getInverseTransposeWorldMatrix(PrimaryHit.instanceID);
    float3x3 invMat = transpose(invTransMat);
    float3 localNormal = mul(Normal, invMat);
    float3 prevNormal = mul(localNormal, float3x3(gScene.getPrevInverseTransposeWorldMatrix(PrimaryHit.instanceID)));
    
    float3 Origin = prevL;
    float3 Direction = normalize(prevB - prevL);

    float4 intersection = rayPlaneIntersect(Origin, Direction, prevS, prevNormal);

    float4 prevX = mul(intersection, gScene.camera.data.prevViewProjMatNoJitter);

    return calcMotionVector(X, prevX, RenderTargetDim) + float2(gScene.camera.data.jitterX, -gScene.camera.data.jitterY);


    //float2 MotionVector = calcMotionVector(X, prevX, RenderTargetDim) + float2(gScene.camera.data.jitterX, -gScene.camera.data.jitterY);
    //if (any(isnan(MotionVector)))
    //gShadowMotionVector[PixelIndex] = intersection;
    //return MotionVector;
}


//calculate shadow motion vector as paper ray tracing gems2 
//float2 calcShadowMotionVectorOld(uint2 pixelIndex, float4 hitPos, float4 hitNormal, uint hitMeshID,
//                            float4 blockerPos, uint blockerMeshID, float4 lightPos,uint lightMehsID, float renderTargetDim)
//{
//    //for each pixel in shadow (in image space)
//    float2 X = pixelIndex + float2(0.5f, 0.5f);

//    //all in world space
//    float4 S = hitPos;
//    float4 B = blockerPos;
//    float4 L = lightPos;

//    const GeometryInstanceID BInstanceID = { blockerMeshID };
//    const GeometryInstanceID LInstanceID = { lightMehsID }; //TODO : is Light a Mesh???
//    const GeometryInstanceID SInstanceID = { hitMeshID };
    
//    float4 LocalB = mul(B, transpose(gScene.getInverseTransposeWorldMatrix(BInstanceID))); //inverse world matrix to local
//    float4 LocalL = mul(L, transpose(gScene.getInverseTransposeWorldMatrix(LInstanceID)));

//    float4 prevB = mul(LocalB, gScene.getPrevWorldMatrix(BInstanceID)); //trans from local to pre world space 
//    float4 prevL = mul(LocalL, gScene.getPrevWorldMatrix(LInstanceID));
  
//    float4 Origin = prevL;
//    float4 Direction = prevB - prevL;

//    float4x4 invTransMat = gScene.getInverseTransposeWorldMatrix(SInstanceID);
//    float4x4 invT = transpose(invTransMat);
//    //float3x3 invN = inverse(float3x3(gScene.getInverseTransposeWorldMatrix(SInstanceID)));
//    float3x3 invN = invT;
    
//    float3 LocalNormal = mul(hitNormal.xyz, invN); 
//    float4 LocalS = mul(S, invT);

//    float4 prevNormal = float4(mul(LocalNormal, float3x3(gScene.loadPrevInverseTransposeWorldMatrix(SInstanceID))),1.0f);
//    float4 preS = mul(LocalS, gScene.getPrevWorldMatrix(SInstanceID));
    
//    float4 intersection = rayPlaneIntersect(Origin, Direction, preS, prevNormal);

//    float4 prevX = mul(intersection, gScene.camera.data.prevViewProjMatNoJitter); 

//    return calcMotionVector(X, prevX, renderTargetDim) + float2(gScene.camera.data.jitterX,-gScene.camera.data.jitterY);
//}


//lg
float2 calcShadowMotionVector( uint2 PixelIndex, const PackedHitInfo packedPrimaryHitInfo, const PackedHitInfo packedShadowHitInfo,
                               float3 Normal, float3 LightPosW, float2 RenderTargetDim)
{
    const TriangleHit PrimaryHit = HitInfo(packedPrimaryHitInfo).getTriangleHit();
    const TriangleHit ShadowHit = HitInfo(packedShadowHitInfo).getTriangleHit();

    return calcShdowMotionVectorInternel(PixelIndex, PrimaryHit, Normal, ShadowHit, LightPosW, RenderTargetDim);
}


float4 evalShadingAndShadow(const ShadingData sd, float3 rayOrigin, inout SampleGenerator sg, out PackedHitInfo packedShadowHitInfo, out float4 LightPos )
{
    const uint lightCount = gScene.getLightCount();
    if (lightCount == 0) return float4(0,0,0,0);

    // Pick one of the analytic light sources randomly with equal probability.
    const uint lightIndex = min(uint(sampleNext1D(sg) * lightCount), lightCount - 1);
    float invPdf = lightCount; // Light selection pdf = 1.0 / lightCount. 

    // Sample local light source.
    AnalyticLightSample ls;
    //bool valid = sampleLight(rayOrigin, gScene.getLight(lightIndex), sg, ls);
    bool valid = sampleLight(rayOrigin, gScene.getLight(lightIndex), sg, ls);

    //lg
    LightPos.xyz = ls.posW; 
    LightPos.w = float(lightCount);
    //lg
    
    // Reject sample if lower hemisphere.
    //if (!valid || dot(ls.dir, sd.N) <= kMinCosTheta) return float4(0,0,0,0); //todo : may cull light and do not trace ray

    // Test visibility by tracing a shadow ray.
    bool V = traceShadowRay(rayOrigin, ls.dir, ls.distance, packedShadowHitInfo); //lg
    float visibility = V ? 1.0 : 0.0;

    if (!valid || dot(ls.dir, sd.N) <= kMinCosTheta)  return float4(0, 0, 0, V); //lg
    
    float3 color = gData.standardMaterial.eval(sd, ls.dir) * ls.Li * invPdf;
    float4 result = float4(color, visibility);
    return result;
}

/** Processes a hit point to generate a scatter ray or terminate.
    This function generates a cosine-weighted direction over the hemisphere.
    \param[in] sd Shading data.
    \param[in] rayOrigin Ray origin for the new ray.
    \param[in] rayData Ray payload.
    \return True if the path continues.
*/
bool generateScatterRay(const ShadingData sd, float3 rayOrigin, inout ScatterRayData rayData)
{
    // Generate scatter ray as cosine-weighted direction over the hemisphere.
    float pdf = 0.f;
    float2 u = sampleNext2D(rayData.sg);
    float3 wi = sample_cosine_hemisphere_concentric(u, pdf);
    wi = sd.fromLocal(wi);

    rayData.origin = rayOrigin;
    rayData.direction = wi;
    rayData.thp *= pdf > 0.f ? gData.standardMaterial.eval(sd, wi) / pdf : float3(0);
    return any(rayData.thp > 0.f);
}

/** ********************* Ray index 0: Scatter ray ************************ */

[shader("miss")]
void scatterMiss(inout ScatterRayData rayData : SV_RayPayload)
{
    // Ray missed the scene. Mark the ray as terminated.
    rayData.terminated = true;

    // Add contribution from distant light (env map) in this direction.
    if (kUseEnvLight && (kComputeDirect || rayData.pathLength > 0))
    {
        float3 Le = gScene.envMap.eval(WorldRayDirection());
        rayData.radiance += rayData.thp * Le;
    }
}

[shader("anyhit")]
void scatterAnyHit(inout ScatterRayData rayData : SV_RayPayload, BuiltInTriangleIntersectionAttributes attribs : SV_IntersectionAttributes)
{
    // Alpha test for non-opaque geometry.
    GeometryInstanceID instanceID = getGeometryInstanceID();
    VertexData v = getVertexData(instanceID, PrimitiveIndex(), attribs);
    const uint materialID = gScene.getMaterialID(instanceID);
    if (alphaTest(v, gScene.materials[materialID], gScene.materialResources[materialID], 0.f)) IgnoreHit();
}

[shader("closesthit")]
void scatterClosestHit(inout ScatterRayData rayData : SV_RayPayload, BuiltInTriangleIntersectionAttributes attribs : SV_IntersectionAttributes)
{
    // Evaluate Falcor's material parameters at the hit point.
    // TODO: Implement texLOD to enable texture filtering in prepareShadingData().
    GeometryInstanceID instanceID = getGeometryInstanceID();
    VertexData v = getVertexData(instanceID, PrimitiveIndex(), attribs);
    const uint materialID = gScene.getMaterialID(instanceID);
    ShadingData sd = prepareShadingData(v, materialID, gScene.materials[materialID], gScene.materialResources[materialID], -WorldRayDirection(), 0.f);

    // Add emitted light.
    if (kUseEmissiveLights && (kComputeDirect || rayData.pathLength > 0))
    {
        rayData.radiance += rayData.thp * sd.emissive;
    }

    // Check whether to terminate based on max depth.
    if (rayData.pathLength >= kMaxBounces)
    {
        rayData.terminated = true;
        return;
    }

    // Compute ray origin for new rays spawned from the hit.
    float3 rayOrigin = sd.computeNewRayOrigin();

    // Add contribution of direct light from analytic lights.
    if (kUseAnalyticLights)
    {
        float3 Lr = evalDirectAnalytic(sd, rayOrigin, rayData.sg);
        rayData.radiance += rayData.thp * Lr;
    }

    // Generate scatter ray for the next path segment.
    // The raygen shader will continue the path based on the returned payload.
    if (!generateScatterRay(sd, rayOrigin, rayData))
    {
        rayData.terminated = true;
        return;
    }

    rayData.pathLength++;
}

/************************** Ray index 1: Shadow ray ************************ */

[shader("miss")]
void shadowMiss(inout ShadowRayData rayData : SV_RayPayload)
{
    // The miss shader is executed if the ray misses all geometry. Mark as visible.
    rayData.visible = true;
}

[shader("anyhit")]
void shadowAnyHit(inout ShadowRayData rayData : SV_RayPayload, BuiltInTriangleIntersectionAttributes attribs : SV_IntersectionAttributes)
{
    // Alpha test for non-opaque geometry.
    GeometryInstanceID instanceID = getGeometryInstanceID();
    VertexData v = getVertexData(instanceID, PrimitiveIndex(), attribs);
    const uint materialID = gScene.getMaterialID(instanceID);
    if (alphaTest(v, gScene.materials[materialID], gScene.materialResources[materialID], 0.f)) IgnoreHit();
}

[shader("closesthit")]
void shadowClosethit(inout ShadowRayData rayData : SV_RayPayload, BuiltInTriangleIntersectionAttributes attribs : SV_IntersectionAttributes)
{
    rayData.visible = false;

    TriangleHit trangleHit;
    trangleHit.instanceID = getGeometryInstanceID();
    trangleHit.primitiveIndex = PrimitiveIndex();
    trangleHit.barycentrics = attribs.barycentrics;
    rayData.packedHitInfo = HitInfo(trangleHit).pack();
}

/** ******************************** RayGen ******************************** */

/** This is the entry point for the minimal path tracer.

    One path per pixel is generated, which is traced into the scene.
    The path tracer is written as a for-loop over path segments.

    Built-in light sources (point, directional) are sampled explicitly at each
    path vertex. The contributions from area lights (env map and mesh lights)
    are explicitly added by the scatter ray hit/miss shaders.
*/
[shader("raygeneration")]
void rayGen()
{
    uint2 launchIndex = DispatchRaysIndex().xy;  //pixel index
    uint2 launchDim = DispatchRaysDimensions().xy;

    float3 outColor = float3(0, 0, 0);

    const float3 rayDir = getPrimaryRayDir(launchIndex, launchDim, gScene.camera);
    const float4 worldPos = gWorldPosition[launchIndex];

    if (worldPos.w != 0.f)  // Using w to indicate valid geometry for now
    {
        // Pixel represents a valid primary hit. Compute its contribution.

        // Load geometry parameters from G-buffer.
        // TODO: Load (u,v) channel if it exists.
        float3 normal = gWorldShadingNormal[launchIndex].xyz;
        float4 tangent = is_valid(gWorldShadingTangent) ? gWorldShadingTangent[launchIndex] : float4(perp_stark(normal), 1.f);
        float3 faceNormal = gWorldFaceNormal[launchIndex].xyz;
        GeometryParams geoParams = prepareGeometryParams(worldPos.xyz, -rayDir, normal, tangent, faceNormal); //load data and normalize (viewDir,normal,tangent)

        // Load material parameters from G-buffer.
        MaterialParams matParams = loadMaterialParams(launchIndex);

        // Prepare ShadingData struct.
        ShadingData sd = prepareShadingData(geoParams, matParams);

        // Create sample generator.
        SampleGenerator sg = SampleGenerator.create(launchIndex, gFrameCount);

        // Advance the generator to the first available dimension.
        // TODO: This is potentially expensive. We may want to store/restore the state from memory if it becomes a problem.
        for (uint i = 0; i < gPRNGDimension; i++) sampleNext1D(sg);

        // Compute ray origin for new rays spawned from the G-buffer.
        const float3 rayOrigin = sd.computeNewRayOrigin(); //according to  hitPos,

        if (kComputeDirect)
        {
            // Always output directly emitted light, independent of whether emissive materials are treated as light sources or not.
            outColor += sd.emissive;

            // Add contribution of direct light from analytic lights.
            // Light probe and mesh lights are handled by the scatter ray hit/miss shaders.
            //outColor += kUseAnalyticLights ? evalDirectAnalytic(sd, rayOrigin, sg) : float3(0, 0, 0);
            PackedHitInfo packedShadowHitInfo; //lg
            float4 LightPosW = float4(0);
            
            float4 result = evalShadingAndShadow(sd, rayOrigin, sg, packedShadowHitInfo, LightPosW); //lg

            outColor += result.xyz;
            gOutputVisibility[launchIndex] = result.w;

            //lg
            PackedHitInfo packedPrimaryHitInfo = gVBuffer[launchIndex];
            
            if (result.w > 0.f) //visiblity > 0. in shadow
            {
                gShadowMotionVector[launchIndex] = float4(0.0f, 0.0f, 0.0f, 1.0f); //todo for test

            }
            else
            {
                float2 ShadowMotionVector = calcShadowMotionVector(launchIndex, packedPrimaryHitInfo, packedShadowHitInfo,
                                                                   normal, LightPosW.xyz, float2(launchDim));


                
                gShadowMotionVector[launchIndex] = float4(ShadowMotionVector, 1.0f, 1.0f);
                //gOutputColor[launchIndex] = float4(ShadowMotionVector, 1.0f, 1.0f);
            }


            float3 ColorWithShadow = outColor * result.w;
            gTest[launchIndex] = float4(ColorWithShadow, 1.0f);
            
        }
    }
    else
    {
        // Background pixel.
        outColor = kUseEnvBackground ? gScene.envMap.eval(rayDir) : kDefaultBackgroundColor;
        gOutputVisibility[launchIndex] = 0.0;
        gTest[launchIndex] = float4(outColor, 1.0f);
    }

    //test compute pos
    //PackedHitInfo packedPrimaryHitInfo2 = gVBuffer[launchIndex];
    //const TriangleHit hitInfo = HitInfo(packedPrimaryHitInfo2).getTriangleHit();

    //const VertexData TestVertex = gScene.getVertexData(hitInfo);
    //float3 prevPos = gScene.getPrevPosW(hitInfo);
    //gOutputColor[launchIndex] = float4(prevPos, 1);
    //const uint ligthCount = gScene.getLightCount();
    //float count = float(ligthCount);
    //gOutputColor[launchIndex] = float4(count, count, count, 1);
    //end test

    gOutputColor[launchIndex] = float4(outColor, 1);
    
}
