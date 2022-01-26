from falcor import *

def render_graph_STSMRenderGraph():
    g = RenderGraph('STSMRenderGraph')
    loadRenderPassLibrary('ErrorMeasurePass.dll')
    loadRenderPassLibrary('Utils.dll')
    loadRenderPassLibrary('CSM.dll')
    loadRenderPassLibrary('BSDFViewer.dll')
    loadRenderPassLibrary('MinimalPathTracer.dll')
    loadRenderPassLibrary('AccumulatePass.dll')
    loadRenderPassLibrary('RayTrac.dll')
    loadRenderPassLibrary('Antialiasing.dll')
    loadRenderPassLibrary('BlitPass.dll')
    loadRenderPassLibrary('DebugPasses.dll')
    loadRenderPassLibrary('SimplePostFX.dll')
    loadRenderPassLibrary('ForwardLightingPass.dll')
    loadRenderPassLibrary('DepthPass.dll')
    loadRenderPassLibrary('SpatioTemporalFilter.dll')
    loadRenderPassLibrary('SkyBox.dll')
    loadRenderPassLibrary('FLIPPass.dll')
    loadRenderPassLibrary('SVGFPass.dll')
    loadRenderPassLibrary('GBuffer.dll')
    loadRenderPassLibrary('ImageLoader.dll')
    loadRenderPassLibrary('MegakernelPathTracer.dll')
    loadRenderPassLibrary('MotionedShadow.dll')
    loadRenderPassLibrary('MergePass.dll')
    loadRenderPassLibrary('OptixDenoiser.dll')
    loadRenderPassLibrary('PassLibraryTemplate.dll')
    loadRenderPassLibrary('PixelInspectorPass.dll')
    loadRenderPassLibrary('SceneDebugger.dll')
    loadRenderPassLibrary('ShadowTaaPass.dll')
    loadRenderPassLibrary('SpatioTemporalSM.dll')
    loadRenderPassLibrary('SSAO.dll')
    loadRenderPassLibrary('TemporalDelayPass.dll')
    loadRenderPassLibrary('TestPasses.dll')
    loadRenderPassLibrary('ToneMapper.dll')
    loadRenderPassLibrary('VPathTracer.dll')
    loadRenderPassLibrary('WhittedRayTracer.dll')
    STSM_MultiViewShadowMapViewWarp = createPass('STSM_MultiViewShadowMapViewWarp')
    g.addPass(STSM_MultiViewShadowMapViewWarp, 'STSM_MultiViewShadowMapViewWarp')
    GBufferRaster = createPass('GBufferRaster', {'samplePattern': SamplePattern.Center, 'sampleCount': 16, 'useAlphaTest': True, 'adjustShadingNormals': True, 'forceCullMode': False, 'cull': CullMode.CullBack})
    g.addPass(GBufferRaster, 'GBufferRaster')
    STSM_CalculateVisibility = createPass('STSM_CalculateVisibility')
    g.addPass(STSM_CalculateVisibility, 'STSM_CalculateVisibility')
    STSM_ShadowMapSelector = createPass('STSM_ShadowMapSelector')
    g.addPass(STSM_ShadowMapSelector, 'STSM_ShadowMapSelector')
    STSM_MultiViewShadowMapRasterize = createPass('STSM_MultiViewShadowMapRasterize')
    g.addPass(STSM_MultiViewShadowMapRasterize, 'STSM_MultiViewShadowMapRasterize')
    MS_Visibility = createPass('MS_Visibility')
    g.addPass(MS_Visibility, 'MS_Visibility')
    SVGFPass = createPass('SVGFPass', {'Enabled': True, 'Iterations': 4, 'FeedbackTap': 1, 'VarianceEpsilon': 9.999999747378752e-05, 'PhiColor': 10.0, 'PhiNormal': 128.0, 'Alpha': 0.05000000074505806, 'MomentsAlpha': 0.20000000298023224})
    g.addPass(SVGFPass, 'SVGFPass')
    VPathTracer_ = createPass('VPathTracer', {'params': PathTracerParams(samplesPerPixel=1, lightSamplesPerVertex=1, maxBounces=3, maxNonSpecularBounces=3, useVBuffer=1, useAlphaTest=1, adjustShadingNormals=0, forceAlphaOne=1, clampSamples=0, clampThreshold=10.0, specularRoughnessThreshold=0.25, useBRDFSampling=1, useNEE=1, useMIS=1, misHeuristic=1, misPowerExponent=2.0, useRussianRoulette=0, probabilityAbsorption=0.20000000298023224, useFixedSeed=0, useNestedDielectrics=1, useLightsInDielectricVolumes=0, disableCaustics=0, rayFootprintMode=0, rayConeMode=2, rayFootprintUseRoughness=0), 'sampleGenerator': 1, 'emissiveSampler': EmissiveLightSamplerType.LightBVH, 'uniformSamplerOptions': EmissiveUniformSamplerOptions(), 'lightBVHSamplerOptions': LightBVHSamplerOptions(buildOptions=LightBVHBuilderOptions(splitHeuristicSelection=SplitHeuristic.BinnedSAOH, maxTriangleCountPerLeaf=10, binCount=16, volumeEpsilon=0.0010000000474974513, splitAlongLargest=False, useVolumeOverSA=False, useLeafCreationCost=True, createLeavesASAP=True, allowRefitting=True, usePreintegration=True, useLightingCones=True), useBoundingCone=True, useLightingCone=True, disableNodeFlux=False, useUniformTriangleSampling=True, solidAngleBoundMethod=SolidAngleBoundMethod.Sphere)})
    g.addPass(VPathTracer_, 'VPathTracer_')
    VBufferRT = createPass('VBufferRT', {'samplePattern': SamplePattern.Center, 'sampleCount': 16, 'useAlphaTest': True, 'adjustShadingNormals': True, 'forceCullMode': False, 'cull': CullMode.CullBack, 'useTraceRayInline': False})
    g.addPass(VBufferRT, 'VBufferRT')
    g.addEdge('GBufferRaster.depth', 'STSM_CalculateVisibility.Depth')
    g.addEdge('STSM_ShadowMapSelector.ShadowMap', 'STSM_CalculateVisibility.ShadowMap')
    g.addEdge('STSM_MultiViewShadowMapViewWarp.ShadowMapSet', 'STSM_ShadowMapSelector.ViewWarp')
    g.addEdge('STSM_MultiViewShadowMapRasterize.ShadowMapSet', 'STSM_ShadowMapSelector.Rasterize')
    g.addEdge('STSM_MultiViewShadowMapRasterize.IdSet', 'MS_Visibility.Ids')
    g.addEdge('STSM_ShadowMapSelector.ShadowMap', 'MS_Visibility.SMs')
    g.addEdge('STSM_CalculateVisibility.LightUv', 'MS_Visibility.LOffs')
    g.addEdge('GBufferRaster.pnFwidth', 'SVGFPass.PositionNormalFwidth')
    g.addEdge('GBufferRaster.linearZ', 'SVGFPass.LinearZ')
    g.addEdge('GBufferRaster.emissive', 'SVGFPass.Emission')
    g.addEdge('GBufferRaster.posW', 'SVGFPass.WorldPosition')
    g.addEdge('GBufferRaster.normW', 'SVGFPass.WorldNormal')
    g.addEdge('STSM_CalculateVisibility.Visibility', 'SVGFPass.Color')
    g.addEdge('MS_Visibility.smv', 'SVGFPass.MotionVec')
    g.addEdge('VPathTracer_.albedo', 'SVGFPass.Albedo')
    g.addEdge('VBufferRT.vbuffer', 'VPathTracer_.vbuffer')
    g.addEdge('VBufferRT.mvec', 'VPathTracer_.MVec')
    g.markOutput('MS_Visibility.vis')
    return g

STSMRenderGraph = render_graph_STSMRenderGraph()
try: m.addGraph(STSMRenderGraph)
except NameError: None
