#include "Utils/Math/MathConstants.slangh"

import Scene.Raytracing;
import Scene.HitInfo;
import Utils.Math.MathHelpers;
import Utils.Sampling.SampleGenerator;
import Experimental.Scene.Material.StandardMaterial;
import Experimental.Scene.Material.MaterialHelpers;
import Experimental.Scene.Lights.LightHelpers;

cbuffer CB
{
    uint gFrameCount; // Frame count since scene was loaded.
    uint gPRNGDimension; // First available PRNG dimension.
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

Texture2D<float4> gMotionVector;//traditional motion vector
Texture2D<PackedHitInfo> gVBuffer;//lg

// Outputs
RWTexture2D<float4> gOutputColor; //Only Visibility Now
RWTexture2D<float4> gShadowMotionVector;//Shadow Motion vector

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


/**Payload for shadow motion vector
*/

struct ShadowRayForMotionVectorInfo
{
    bool visible;
    PackedHitInfo packedHitInfo;
    bool isLightInLowerHemiphere;
    
    float4 LightPos;
    float4 PrevLightPos;
    
};

/** Payload for scatter ray (80B).
*/
struct ScatterRayData
{
    float3 radiance; ///< Accumulated outgoing radiance from path.
    bool terminated; ///< Set to true when path is terminated.
    float3 thp; ///< Current path throughput. This is updated at each path vertex.
    uint pathLength; ///< Path length in number of path segments (0 at origin, 1 at first secondary hit, etc.). Max 2^31.
    float3 origin; ///< Next path segment origin.
    uint _pad0;
    float3 direction; ///< Next path segment direction.
    uint _pad1;

    SampleGenerator sg; ///< Per-ray state for the sample generator (up to 16B).

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




/** LG shadow motion vector calculte process
*/


//lg:ray and plane intersect helper
bool intersectRayPlane(const float3 rayOrign, const float3 rayDir, const float3 PlanePoint, const float3 PlaneNormal, out float t)
{
    float PlaneData = dot(PlanePoint, PlaneNormal); //all points on the plane dot normal = constant
    float RayODotN = dot(rayOrign, PlaneNormal);
    float DirDotN = dot(rayDir, PlaneNormal);

    t = 0.0f;
    if (DirDotN == 0.f) //ray paralle plane
        return false;
    
    t = (PlaneData - RayODotN) / DirDotN;

    return t >= 0.f; //t > 0 has intersection
}

//lg
float4 rayPlaneIntersect(float3 origin, float3 dir, float3 Pos, float3 Normal)
{
    float ResultT = 0.0f;
    float4 Intersection = float4(0.f, 0.f, 0.f, 0.f);
    if (intersectRayPlane(origin, dir, Pos, Normal, ResultT))
    {
        Intersection.xyz = origin + ResultT * dir;
        Intersection.w = 1.0f;
    }
    return Intersection;
}

//todo check if is right
//2021-zeng-temporal reliable motion vector for ray tracing
float4 calcShadowMotionVector(uint2 pixelIndex, const TriangleHit PrimaryHit, float3 Normal,
                              const TriangleHit ShadowHit, float3 PrevLightPos, float2 RenderTargetDim)
{
    float2 X = pixelIndex + float2(0.5f, 0.5f);
    
    float3 prevS = gScene.getPrevPosW(PrimaryHit);
    float3 prevB = gScene.getPrevPosW(ShadowHit);
    float3 prevL = PrevLightPos;

    //prevNormal
    float3x3 invTransMat = gScene.getInverseTransposeWorldMatrix(PrimaryHit.instanceID);
    float3x3 invMat = transpose(invTransMat);
    float3 localNormal = mul(Normal, invMat);
    float3 prevNormal = mul(localNormal, float3x3(gScene.getPrevInverseTransposeWorldMatrix(PrimaryHit.instanceID)));
    
    float3 Origin = prevL;
    float3 Direction = normalize(prevB - prevL);

    float4 intersection = rayPlaneIntersect(Origin, Direction, prevS, prevNormal);
    
    float4 prevX = mul(intersection, gScene.camera.data.prevViewProjMatNoJitter);

    
    float2 VMotion = calcMotionVector(X, prevX, RenderTargetDim) + float2(gScene.camera.data.jitterX, -gScene.camera.data.jitterY);

    return float4(VMotion, 0, 1);
}


float4 getVMotionVec(uint2 PixelIndex, const HitInfo PrimaryHit, const ShadowHitInfo ShadowHitData, const ShadingData sd, float2 RenderTargetDim)
{
    if (ShadowHitData.Visibility > 0.f) //not in shadow
    {
        return float4(0, 0, 1, 1);
    }
    else
    { //in shadow
        if (ShadowHitData.isLightInLowerHemiphere) //is in lower hemsisphere
        {
            return float4(gMotionVectors[PixelIndex].xy, 1, 0); //todo:need to think more //test
        }
        else
        {
            return calcVMotionVec(PixelIndex, PrimaryHit, ShadowHitData, sd, RenderTargetDim);
        }
    }
}

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
bool traceShadowRay(float3 origin, float3 dir, float distance)
{
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = dir;
    ray.TMin = 0.f;
    ray.TMax = distance;

    ShadowRayData rayData;
    rayData.visible = false; // Set to true by miss shader if ray is not terminated before
    TraceRay(gScene.rtAccel, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xff /* instanceInclusionMask */, 1 /* hitIdx */, rayTypeCount, 1 /* missIdx */, ray, rayData);

    return rayData.visible;
}


//lg start
// modify logic to store the shadow ray hit info
void LG_traceShadowRay(float3 origin, float3 dir, float distance, out ShadowRayData rayData)
{
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = dir;
    ray.TMin = 0.f;
    ray.TMax = distance;

    rayData.visible = false;

    uint rayFlags = RAY_FLAG_NONE;//0
    TraceRay(gScene.rtAccel, rayFlags, 0xff /*instanceInclusionMask*/, 1 /*hitIndex*/, rayTypeCount, 1 /*missIndex*/, ray, rayData);

}
//lg end


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

    uint rayFlags = 0; // TODO: Set cull mode from the app
    TraceRay(gScene.rtAccel, rayFlags, 0xff /* instanceInclusionMask */, 0 /* hitIdx */, rayTypeCount, 0 /* missIdx */, ray, rayData);
}

/** Evaluates the direct illumination from analytic lights.
    This function samples Falcor's light list uniformly with one shadow ray.
    \param[in] sd Shading data.
    \param[in] rayOrigin Ray origin for the shadow ray.
    \param[in] sg SampleGenerator object.
    \return Outgoing radiance in view direction.
*/
float3 evalDirectAnalytic(const ShadingData sd, float3 rayOrigin, inout SampleGenerator sg)
{
    const uint lightCount = gScene.getLightCount();
    if (lightCount == 0)
        return float3(0);

    // Pick one of the analytic light sources randomly with equal probability.
    const uint lightIndex = min(uint(sampleNext1D(sg) * lightCount), lightCount - 1);
    float invPdf = lightCount; // Light selection pdf = 1.0 / lightCount.

    // Sample local light source.
    AnalyticLightSample ls;
    bool valid = sampleLight(rayOrigin, gScene.getLight(lightIndex), sg, ls);

    // Reject sample if lower hemisphere.
    if (!valid || dot(ls.dir, sd.N) <= kMinCosTheta)
        return float3(0);

    bool V = traceShadowRay(rayOrigin, ls.dir, ls.distance);
    return V ? gData.standardMaterial.eval(sd, ls.dir) * ls.Li * invPdf : float3(0); //lg
}


//lg
ShadowRayForMotionVectorInfo LG_evalDirectAnalytic(const ShadingData sd, float3 rayOrigin, inout SampleGenerator sg)
{
    //NEW LOGIC
    ShadowRayForMotionVectorInfo SRMVInfo;

    SRMVInfo.visible = false;
    SRMVInfo.isLightInLowerHemiphere = false;

    const uint lightCount = gScene.getLightCount();
    if (lightCount == 0){
        return SRMVInfo;
    }

    const uint lightIndex = min(uint(sampleNext1D(sg) * lightCount), lightCount - 1);
    float invPdf = lightCount; // Light selection pdf = 1.0 / lightCount.

    //Sample local light source.
    AnalyticLightSample ls;
    bool valid = sampleLight(rayOrigin, gScene.getLight(lightIndex), sg, ls);

    //reject  sample if lower hemisphere
    if (!valid || dot(ls.dir, sd.N) <= kMinCosTheta){
        SRMVInfo.isLightInLowerHemiphere = true;
        return SRMVInfo;
    }

    ShadowRayData RayData;
    RayData.visible = false;
    
    LG_traceShadowRay(rayOrigin, ls.dir, ls.distance, RayData);

    SRMVInfo.visible = RayData.visible;
    SRMVInfo.packedHitInfo = RayData.packedHitInfo;
    SRMVInfo.LightPos = ls.posW;

    //todo preLightPos;
    
    return SRMVInfo;
    
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
    if (alphaTest(v, gScene.materials[materialID], gScene.materialResources[materialID], 0.f))
        IgnoreHit();
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
    if (alphaTest(v, gScene.materials[materialID], gScene.materialResources[materialID], 0.f))
        IgnoreHit();
}


//lg
[shader("closesthit")]
void shadowClosestHit(inout ShadowRayData rayData:SV_RayPayload,BuiltInTriangleIntersectionAttributes attribs : SV_IntersectionAttributes)
{

    //store hit information.Note we dont access the materials here.
    TriangleHit triangleHit;
    triangleHit.instanceID = getGeometryInstanceID();
    triangleHit.primitiveIndex = PrimitiveIndex();
    triangleHit.barycentrics = attribs.barycentrics;

    rayData.packedHitInfo = HitInfo(triangleHit).pack();
    rayData.visible = false; //hit blocker == inVisible
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
    uint2 launchIndex = DispatchRaysIndex().xy;
    uint2 launchDim = DispatchRaysDimensions().xy;

    float3 outColor = float3(0, 0, 0);

    const float3 rayDir = getPrimaryRayDir(launchIndex, launchDim, gScene.camera);
    const float4 worldPos = gWorldPosition[launchIndex];


    //lg
    ShadowRayForMotionVectorInfo SRMVInfo;
    float4 ShadowMotionVector;
    
    if (worldPos.w != 0.f)  // Using w to indicate valid geometry for now
    {
        // Pixel represents a valid primary hit. Compute its contribution.

        // Load geometry parameters from G-buffer.
        // TODO: Load (u,v) channel if it exists.
        float3 normal = gWorldShadingNormal[launchIndex].xyz;
        float4 tangent = is_valid(gWorldShadingTangent) ? gWorldShadingTangent[launchIndex] : float4(perp_stark(normal), 1.f);
        float3 faceNormal = gWorldFaceNormal[launchIndex].xyz;
        GeometryParams geoParams = prepareGeometryParams(worldPos.xyz, -rayDir, normal, tangent, faceNormal);

        // Load material parameters from G-buffer.
        MaterialParams matParams = loadMaterialParams(launchIndex);

        // Prepare ShadingData struct.
        ShadingData sd = prepareShadingData(geoParams, matParams);

        // Create sample generator.
        SampleGenerator sg = SampleGenerator.create(launchIndex, gFrameCount);

        // Advance the generator to the first available dimension.
        // TODO: This is potentially expensive. We may want to store/restore the state from memory if it becomes a problem.
        for (uint i = 0; i < gPRNGDimension; i++)
            sampleNext1D(sg);

        // Compute ray origin for new rays spawned from the G-buffer.
        const float3 rayOrigin = sd.computeNewRayOrigin();

        if (kComputeDirect)
        {
            // Always output directly emitted light, independent of whether emissive materials are treated as light sources or not.
            outColor += sd.emissive;

            // Add contribution of direct light from analytic lights.
            // Light probe and mesh lights are handled by the scatter ray hit/miss shaders.
            // outColor += kUseAnalyticLights ? evalDirectAnalytic(sd, rayOrigin, sg) : float3(0, 0, 0); //lg: calculate visibilty


            if (kUseAnalyticLights) {
                SRMVInfo = LG_evalDirectAnalytic(sd, rayOrigin, sg); //if need shading ? 
            }
            else{
                SRMVInfo.visible = false;
            }

            outColor = SRMVInfo.visible ? float3(1.0, 1.0, 1.0) : float3(0, 0, 0);

        }

        //lg
        //delete scatterRay
        
        // Prepare ray payload.
        //ScatterRayData rayData = ScatterRayData.create(sg);

        // Generate scatter ray.
        //if (!generateScatterRay(sd, rayOrigin, rayData))
        //    rayData.terminated = true;

        // Follow path into the scene and compute its total contribution.
        //for (uint depth = 0; depth <= kMaxBounces && !rayData.terminated; depth++)
        //{
        //    // Trace scatter ray. If it hits geometry, the closest hit shader samples
        //    // direct illumination and generates the next scatter ray.
        //    traceScatterRay(rayData); 
        //} 
        // Store contribution from scatter ray.
        //outColor += rayData.radiance;




        //ShadingData sd2;
        //const HitInfo hit = HitInfo(gVBuffer[launchIndex]);
        //if(hit.isValid() && hit.getType() == HitType::Triangle)
        //{

        //    const TriangleHit triangleHit = hit.getTriangleHit();
        //    const VertexData v = gScene.getVertexData(triangleHit);
        //    const uint materialID = gScene.getMaterialID(triangleHit.instanceID);
        //    sd2 = prepareShadingData(v, materialID, gScene.materials[materialID], gScene.materialResources[materialID], -rayDir, 0.f);
        //}
        //outColor = sd2.posW;



        //lg compute motion vector for shadow ray
        const HitInfo hit = HitInfo(gVBuffer[launchIndex]);
        const TriangleHit triangleHit = hit.getTriangleHit();
        const HitInfo ShadowRayHit = HitInfo(SRMVInfo.packedHitInfo).getTriangleHit();
        
        if(SRMVInfo.visible)//not in shadow
        {
            ShadowMotionVector = float4(0, 0, 1, 1);

        }
        else  //in shadow and compute shadow motion vector
        {
            if(SRMVInfo.isLightInLowerHemiphere) //in lower , no shadow ray
            {
                ShadowMotionVector = float4(gMotionVector[launchIndex].xy, 1, 0);
            }
            else
            {
                ShadowMotionVector = calcShadowMotionVector(launchIndex, triangleHit, sd.N, SRMVInfo.PrevLightPos, launchDim);

            }
        }
        
        
    }
    else
    {
        // Background pixel.
        outColor = kUseEnvBackground ? gScene.envMap.eval(rayDir) : kDefaultBackgroundColor;
    }

    gOutputColor[launchIndex] = float4(outColor, 1);
    gShadowMotionVector[launchIndex] = ShadowMotionVector;
}
