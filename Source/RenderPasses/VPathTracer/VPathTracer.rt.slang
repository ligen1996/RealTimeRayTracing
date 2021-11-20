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

/** Path tracing pass.

    This file contains the entry points for all ray tracing programs.
    We import the path tracer utility functions defined in PathTracer.slang.

    The host sets the compile-time constants in StaticParams.slang.
    It also sets the following defines for optional I/O buffers:

    is_valid_<name> is 1 if buffer with this name is bound, 0 otherwise.
*/
import VPathTracer;
import Utils.Timing.GpuTimer;
import RenderPasses.Shared.PathTracer.LoadShadingData;

ParameterBlock<PathTracerData> gData;


Texture2D<float2> gMotionVectors; //lg

// Outputs (optional)
RWTexture2D<float4> gOutputColor;
RWTexture2D<float4> gOutputAlbedo;
RWTexture2D<uint> gOutputTime;

//LG
RWTexture2D<float4> gOutputVisibility;
RWTexture2D<float4> gOutputVMotionVec;

// Static configuration based on which buffers are bound.
#define is_valid(name) (is_valid_##name != 0)


/** ********************* Ray index 0: Scatter ray ************************ */

[shader("miss")]
void scatterMiss(inout ScatterRayData rayData : SV_RayPayload)
{
}

[shader("anyhit")]
void scatterAnyHit(inout ScatterRayData rayData : SV_RayPayload, BuiltInTriangleIntersectionAttributes attribs : SV_IntersectionAttributes)
{
#if USE_ALPHA_TEST
    // Alpha test for non-opaque geometry.
    GeometryInstanceID instanceID = getGeometryInstanceID();
    VertexData v = getVertexData(instanceID, PrimitiveIndex(), attribs);
    const uint materialID = gScene.getMaterialID(instanceID);
    if (alphaTest(v, gScene.materials[materialID], gScene.materialResources[materialID], 0.f)) IgnoreHit();
#endif
}

[shader("closesthit")]
void scatterClosestHit(inout ScatterRayData rayData : SV_RayPayload, BuiltInTriangleIntersectionAttributes attribs : SV_IntersectionAttributes)
{
    // Store hit information. Note we don't access the materials here.
    TriangleHit triangleHit;
    triangleHit.instanceID = getGeometryInstanceID();
    triangleHit.primitiveIndex = PrimitiveIndex();
    triangleHit.barycentrics = attribs.barycentrics;
    rayData.packedHitInfo = HitInfo(triangleHit).pack();
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
#if USE_ALPHA_TEST
    // Alpha test for non-opaque geometry.
    GeometryInstanceID instanceID = getGeometryInstanceID();
    VertexData v = getVertexData(instanceID, PrimitiveIndex(), attribs);
    const uint materialID = gScene.getMaterialID(instanceID);
    if (alphaTest(v, gScene.materials[materialID], gScene.materialResources[materialID], 0.f)) IgnoreHit();
#endif
}

//lg
[shader("closesthit")]
void shadowClosestHit(inout ShadowRayData rayData : SV_RayPayload, BuiltInTriangleIntersectionAttributes attribs : SV_IntersectionAttributes)
{
    // Store hit information. Note we don't access the materials here.
    TriangleHit triangleHit;
    triangleHit.instanceID = getGeometryInstanceID();
    triangleHit.primitiveIndex = PrimitiveIndex();
    triangleHit.barycentrics = attribs.barycentrics;

    //lg
    rayData.packedHitInfo = HitInfo(triangleHit).pack();
    rayData.visible = false;
}

//lg
//*******************************
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

    return float4(VMotion,0,1);
  
}

//lg
float4 calcVMotionVec(uint2 PixelIndex, const HitInfo PrimaryHit, const ShadowHitInfo ShadowHitData, const ShadingData sd, float2 RenderTargetDim)
{
    const TriangleHit primaryHit = PrimaryHit.getTriangleHit();
    const TriangleHit shadowHit = ShadowHitData.ShadowHit.getTriangleHit();

    return calcShadowMotionVector(PixelIndex, primaryHit, sd.N, shadowHit, ShadowHitData.PreVLightPos, RenderTargetDim); //todo use previous Light Pos
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
/** ******************************** RayGen ******************************** */

/** This is the entry point for the path tracer.

    We generate N paths (= #spp) per pixel, which are traced into the scene.
    The path tracer is written as a for-loop over path segments, where each
    iteration traces a shadow ray for direct illumination and a scatter ray.

    The hit shader for the scatter ray currently generates ray parameters for
    the shadow ray to evaluate direct illumination and generates ray parameters
    for the next scatter ray, which are both returned the raygen shader to be
    traced. This is more efficient than tracing from the hit shader. The max
    recusion depth = 1.
*/
[shader("raygeneration")]
void rayGen()
{
    uint2 launchIndex = DispatchRaysIndex().xy;
    uint2 launchDim = DispatchRaysDimensions().xy;

    logSetPixel(launchIndex);
    printSetPixel(launchIndex);

    GpuTimer timer;
    if (is_valid(gOutputTime)) timer.start();

    float3 outColor = float3(0, 0, 0);
    float3 outAlbedo = float3(0, 0, 0);
    float outAlpha = 0.f;

    HitInfo hit;
    ShadingData sd;
    //lg
    ShadowHitInfo ShadowhitData;
    float outVisibility = 0.f;
    float4 VMotionVec = float4(0);

    //test
    float3 LightPos = float3(0);
    //
    //lg
    
    if (loadShadingData(launchIndex, launchDim, gScene.camera, sd, hit))
    {
        // Pixel represents a primary hit. Compute its contribution.

        // Compute ray origin for new rays spawned from the G-buffer.
        const float3 rayOrigin = sd.computeNewRayOrigin();

        // Loop over samples in pixel.
        [loop]
        for (uint sampleIdx = 0; sampleIdx < kSamplesPerPixel; sampleIdx++)
        {
            // Setup path data.
            PathData path = {};
            path.origin = rayOrigin;
            path.thp = float3(1.f);
            path.hit = hit;

            // Create sample generator.
            uint frameSeed = gData.params.useFixedSeed ? 0 : gData.params.frameCount;
            path.sg = SampleGenerator.create(launchIndex, frameSeed * kSamplesPerPixel + sampleIdx);

            // Advance the generator to the first available dimension.
            // TODO: This is potentially expensive. We may want to store/restore the state from memory if it becomes a problem.
            for (uint i = 0; i < gData.params.prngDimension; i++) sampleNext1D(path.sg);

            // TODO: Use (kRayFootprintMode != TexLODMode::Mip0) when slang is fixed.
            if (!(kRayFootprintMode == TexLODMode::Mip0))
            {
                // Create the ray footprint data for TexLOD.
                path.rayFootprint = RayFootprint.create(hit.getTriangleHit(), launchIndex, launchDim, rayOrigin, gScene.camera.getPosition(), sd.faceN, sd.N, gData.params.screenSpacePixelSpreadAngle, sd.linearRoughness, path.isSpecular());
            }

            // Trace the path.
            //tracePath(gData, sd, path);//origin way
            //lg
            tracePathOnlyDirect(gData, sd, path, ShadowhitData);
            outVisibility += ShadowhitData.Visibility;
            //lg
            
            logPathLength(path.length);

            // Accumulate after clamping.
            // Note the comparison is written so that NaNs propagate (unless the compiler rewrites it).
            // TODO: Check the generated code that this is the case.
            outColor += gData.params.clampSamples && path.L > gData.params.clampThreshold ? gData.params.clampThreshold : path.L;
        }

        // We're done accumulating over all samples.
        const float invSpp = 1.f / kSamplesPerPixel;
        outColor *= invSpp;
        outAlbedo = sd.diffuse + sd.specular;
        outAlpha = 1.f;

        
        //lg
        outVisibility *= invSpp;
        VMotionVec = getVMotionVec(launchIndex, hit, ShadowhitData, sd, launchDim); //get Visibility motion vector
        //lg
    }
    else
    {
        // Background pixel.
        outColor = evalBackground(-sd.V);
        outAlbedo = outColor.rgb;
        outAlpha = kForceAlphaOne ? 1.f : 0.f;
    }

    // DEBUG
    assert(!any(isnan(outColor)));
    assert(!any(isnan(VMotionVec)));//lg

    
    // Write outputs.
    // These are all optional so using compile-time checks to decide which ones to write.
    if (is_valid(gOutputColor)) gOutputColor[launchIndex] = float4(outColor, outAlpha);
    if (is_valid(gOutputAlbedo)) gOutputAlbedo[launchIndex] = float4(outAlbedo, 1);

    // Write time.
    if (is_valid(gOutputTime)) gOutputTime[launchIndex] = timer.getElapsed();

    //LG  Write Visibility and V motion vector
    if (is_valid(gOutputVisibility))  gOutputVisibility[launchIndex] = float4(outVisibility);
    if (is_valid(gOutputVMotionVec))  gOutputVMotionVec[launchIndex] = VMotionVec;    
}
