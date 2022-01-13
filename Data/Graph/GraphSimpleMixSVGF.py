from falcor import *

def render_graph_STSMRenderGraph():
    g = RenderGraph('STSMRenderGraph')
    loadRenderPassLibrary('SceneDebugger.dll')
    loadRenderPassLibrary('BSDFViewer.dll')
    loadRenderPassLibrary('AccumulatePass.dll')
    loadRenderPassLibrary('SpatioTemporalSM.dll')
    loadRenderPassLibrary('DepthPass.dll')
    loadRenderPassLibrary('VPathTracer.dll')
    loadRenderPassLibrary('Antialiasing.dll')
    loadRenderPassLibrary('BlitPass.dll')
    loadRenderPassLibrary('DebugPasses.dll')
    loadRenderPassLibrary('CSM.dll')
    loadRenderPassLibrary('ErrorMeasurePass.dll')
    loadRenderPassLibrary('SkyBox.dll')
    loadRenderPassLibrary('SimplePostFX.dll')
    loadRenderPassLibrary('FLIPPass.dll')
    loadRenderPassLibrary('PixelInspectorPass.dll')
    loadRenderPassLibrary('ForwardLightingPass.dll')
    loadRenderPassLibrary('GBuffer.dll')
    loadRenderPassLibrary('RayTrac.dll')
    loadRenderPassLibrary('ImageLoader.dll')
    loadRenderPassLibrary('MegakernelPathTracer.dll')
    loadRenderPassLibrary('ToneMapper.dll')
    loadRenderPassLibrary('MergePass.dll')
    loadRenderPassLibrary('MinimalPathTracer.dll')
    loadRenderPassLibrary('OptixDenoiser.dll')
    loadRenderPassLibrary('SpatioTemporalFilter.dll')
    loadRenderPassLibrary('PassLibraryTemplate.dll')
    loadRenderPassLibrary('ShadowTaaPass.dll')
    loadRenderPassLibrary('SSAO.dll')
    loadRenderPassLibrary('SVGFPass.dll')
    loadRenderPassLibrary('TemporalDelayPass.dll')
    loadRenderPassLibrary('TestPasses.dll')
    loadRenderPassLibrary('Utils.dll')
    loadRenderPassLibrary('WhittedRayTracer.dll')
    STSM_MultiViewShadowMapViewWarp = createPass('STSM_MultiViewShadowMapViewWarp')
    g.addPass(STSM_MultiViewShadowMapViewWarp, 'STSM_MultiViewShadowMapViewWarp')
    STSM_BilateralFilter = createPass('STSM_BilateralFilter')
    g.addPass(STSM_BilateralFilter, 'STSM_BilateralFilter')
    GBufferRaster = createPass('GBufferRaster', {'samplePattern': SamplePattern.Center, 'sampleCount': 16, 'useAlphaTest': True, 'adjustShadingNormals': True, 'forceCullMode': False, 'cull': CullMode.CullBack})
    g.addPass(GBufferRaster, 'GBufferRaster')
    STSM_CalculateVisibility = createPass('STSM_CalculateVisibility')
    g.addPass(STSM_CalculateVisibility, 'STSM_CalculateVisibility')
    STSM_ShadowMapSelector = createPass('STSM_ShadowMapSelector')
    g.addPass(STSM_ShadowMapSelector, 'STSM_ShadowMapSelector')
    STSM_MultiViewShadowMapRasterize = createPass('STSM_MultiViewShadowMapRasterize')
    g.addPass(STSM_MultiViewShadowMapRasterize, 'STSM_MultiViewShadowMapRasterize')
    SVGFPass = createPass('SVGFPass', {'Enabled': True, 'Iterations': 4, 'FeedbackTap': 1, 'VarianceEpsilon': 9.999999747378752e-05, 'PhiColor': 10.0, 'PhiNormal': 128.0, 'Alpha': 0.05000000074505806, 'MomentsAlpha': 0.20000000298023224})
    g.addPass(SVGFPass, 'SVGFPass')
    g.addEdge('STSM_MultiViewShadowMapRasterize.ShadowMap', 'STSM_ShadowMapSelector.Rasterize')
    g.addEdge('GBufferRaster.depth', 'STSM_CalculateVisibility.Depth')
    g.addEdge('STSM_ShadowMapSelector.ShadowMap', 'STSM_CalculateVisibility.ShadowMap')
    g.addEdge('STSM_MultiViewShadowMapViewWarp.ShadowMap', 'STSM_ShadowMapSelector.ViewWarp')
    g.addEdge('STSM_CalculateVisibility.Visibility', 'STSM_BilateralFilter.Color')
    g.addEdge('GBufferRaster.normW', 'STSM_BilateralFilter.Normal')
    g.addEdge('GBufferRaster.depth', 'STSM_BilateralFilter.Depth')
    g.addEdge('GBufferRaster.diffuseOpacity', 'SVGFPass.Albedo')
    g.addEdge('GBufferRaster.emissive', 'SVGFPass.Emission')
    g.addEdge('GBufferRaster.posW', 'SVGFPass.WorldPosition')
    g.addEdge('GBufferRaster.normW', 'SVGFPass.WorldNormal')
    g.addEdge('GBufferRaster.pnFwidth', 'SVGFPass.PositionNormalFwidth')
    g.addEdge('GBufferRaster.linearZ', 'SVGFPass.LinearZ')
    g.addEdge('GBufferRaster.mvec', 'SVGFPass.MotionVec')
    g.addEdge('STSM_BilateralFilter.Result', 'SVGFPass.Color')
    g.markOutput('SVGFPass.Filtered image')
    return g

STSMRenderGraph = render_graph_STSMRenderGraph()
try: m.addGraph(STSMRenderGraph)
except NameError: None
