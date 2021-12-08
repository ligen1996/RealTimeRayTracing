from falcor import *

def render_graph_PathTracerACC():
    g = RenderGraph('PathTracerACC')
    loadRenderPassLibrary('AccumulatePass.dll')
    loadRenderPassLibrary('ErrorMeasurePass.dll')
    loadRenderPassLibrary('BSDFViewer.dll')
    loadRenderPassLibrary('Antialiasing.dll')
    loadRenderPassLibrary('BlitPass.dll')
    loadRenderPassLibrary('CSM.dll')
    loadRenderPassLibrary('DebugPasses.dll')
    loadRenderPassLibrary('DepthPass.dll')
    loadRenderPassLibrary('ExampleBlitPass.dll')
    loadRenderPassLibrary('FLIPPass.dll')
    loadRenderPassLibrary('ForwardLightingPass.dll')
    loadRenderPassLibrary('GBuffer.dll')
    loadRenderPassLibrary('ImageLoader.dll')
    loadRenderPassLibrary('WireFramePass.dll')
    loadRenderPassLibrary('MegakernelPathTracer.dll')
    loadRenderPassLibrary('MinimalPathTracer.dll')
    loadRenderPassLibrary('MergePass.dll')
    loadRenderPassLibrary('MyPathTracer.dll')
    loadRenderPassLibrary('OptixDenoiser.dll')
    loadRenderPassLibrary('PassLibraryTemplate.dll')
    loadRenderPassLibrary('RayTrac.dll')
    loadRenderPassLibrary('PixelInspectorPass.dll')
    loadRenderPassLibrary('TestPasses.dll')
    loadRenderPassLibrary('SceneDebugger.dll')
    loadRenderPassLibrary('ShadowTaaPass.dll')
    loadRenderPassLibrary('SimplePostFX.dll')
    loadRenderPassLibrary('TemporalDelayPass.dll')
    loadRenderPassLibrary('SkyBox.dll')
    loadRenderPassLibrary('SSAO.dll')
    loadRenderPassLibrary('SVGFPass.dll')
    loadRenderPassLibrary('ToneMapper.dll')
    loadRenderPassLibrary('Utils.dll')
    loadRenderPassLibrary('WhittedRayTracer.dll')
    GBufferRaster = createPass('GBufferRaster', {'samplePattern': SamplePattern.Center, 'sampleCount': 16, 'useAlphaTest': True, 'adjustShadingNormals': True, 'forceCullMode': False, 'cull': CullMode.CullBack})
    g.addPass(GBufferRaster, 'GBufferRaster')
    MyPathTracer = createPass('MyPathTracer', {'params': PathTracerParams(samplesPerPixel=1, lightSamplesPerVertex=1, maxBounces=3, maxNonSpecularBounces=3, useVBuffer=1, useAlphaTest=1, adjustShadingNormals=0, forceAlphaOne=1, clampSamples=0, clampThreshold=10.0, specularRoughnessThreshold=0.25, useBRDFSampling=1, useNEE=1, useMIS=1, misHeuristic=1, misPowerExponent=2.0, useRussianRoulette=0, probabilityAbsorption=0.20000000298023224, useFixedSeed=0, useNestedDielectrics=1, useLightsInDielectricVolumes=0, disableCaustics=0, rayFootprintMode=0, rayConeMode=2, rayFootprintUseRoughness=0), 'sampleGenerator': 1, 'emissiveSampler': EmissiveLightSamplerType.LightBVH, 'uniformSamplerOptions': EmissiveUniformSamplerOptions(), 'lightBVHSamplerOptions': LightBVHSamplerOptions(buildOptions=LightBVHBuilderOptions(splitHeuristicSelection=SplitHeuristic.BinnedSAOH, maxTriangleCountPerLeaf=10, binCount=16, volumeEpsilon=0.0010000000474974513, splitAlongLargest=False, useVolumeOverSA=False, useLeafCreationCost=True, createLeavesASAP=True, allowRefitting=True, usePreintegration=True, useLightingCones=True), useBoundingCone=True, useLightingCone=True, disableNodeFlux=False, useUniformTriangleSampling=True, solidAngleBoundMethod=SolidAngleBoundMethod.Sphere)})
    g.addPass(MyPathTracer, 'MyPathTracer')
    AccumulatePass = createPass('AccumulatePass', {'enabled': True, 'autoReset': True, 'precisionMode': AccumulatePrecision.Single, 'subFrameCount': 0, 'maxAccumulatedFrames': 0})
    g.addPass(AccumulatePass, 'AccumulatePass')
    ToneMapper = createPass('ToneMapper', {'useSceneMetadata': True, 'exposureCompensation': 0.0, 'autoExposure': False, 'filmSpeed': 100.0, 'whiteBalance': False, 'whitePoint': 6500.0, 'operator': ToneMapOp.Aces, 'clamp': True, 'whiteMaxLuminance': 1.0, 'whiteScale': 11.199999809265137, 'fNumber': 1.0, 'shutter': 1.0, 'exposureMode': ExposureMode.AperturePriority})
    g.addPass(ToneMapper, 'ToneMapper')
    g.addEdge('GBufferRaster.vbuffer', 'MyPathTracer.vbuffer')
    g.addEdge('MyPathTracer.color', 'AccumulatePass.input')
    g.addEdge('AccumulatePass.output', 'ToneMapper.src')
    g.markOutput('ToneMapper.dst')
    return g

PathTracerACC = render_graph_PathTracerACC()
try: m.addGraph(PathTracerACC)
except NameError: None