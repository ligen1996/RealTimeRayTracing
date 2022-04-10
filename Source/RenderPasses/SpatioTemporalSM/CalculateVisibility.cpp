#include "CalculateVisibility.h"
#include "Utils/Sampling/SampleGenerator.h"
#include "..\Helper\Helper.h"

namespace
{
    const char kDesc[] = "Calculate Visibility pass";

    // input
    const std::string kDepth = "Depth";
    const std::string kShadowMapSet = "ShadowMap";

    // output
    const std::string kVisibility = "Visibility";
    const std::string kDebug = "Debug";
    const std::string kLightUv = "LightUv";
   
    // shader file path
    const std::string kEsitimationPassfile = "RenderPasses/SpatioTemporalSM/VisibilityPass.ps.slang";
}

Gui::DropdownList STSM_CalculateVisibility::mMapNumList =
{
    Gui::DropdownValue{ 1, std::to_string(1) },
    Gui::DropdownValue{ 3, std::to_string(3) },
    Gui::DropdownValue{ 7, std::to_string(7) },
    Gui::DropdownValue{ 20, std::to_string(20) },
    Gui::DropdownValue{ 256, std::to_string(256) },
};

STSM_CalculateVisibility::STSM_CalculateVisibility()
{
    _ASSERTE(mNumShadowMap <= _MAX_SHADOW_MAP_NUM);
    Program::DefineList Defines;
    Defines.add("SAMPLE_GENERATOR_TYPE", std::to_string(SAMPLE_GENERATOR_UNIFORM));

    mVisibilityPass.pFbo = Fbo::create();
    mVisibilityPass.pPass = FullScreenPass::create(kEsitimationPassfile, Defines);

    Sampler::Desc samplerDesc;
    samplerDesc.setFilterMode(Sampler::Filter::Point, Sampler::Filter::Point, Sampler::Filter::Point).setAddressingMode(Sampler::AddressMode::Border, Sampler::AddressMode::Border, Sampler::AddressMode::Border).setBorderColor(float4(1.0f));
    samplerDesc.setLodParams(0.f, 0.f, 0.f);
    samplerDesc.setComparisonMode(Sampler::ComparisonMode::LessEqual);
    mVisibilityPass.pPointCmpSampler = Sampler::create(samplerDesc);

    samplerDesc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Linear);
    mVisibilityPass.pLinearCmpSampler = Sampler::create(samplerDesc);
}

STSM_CalculateVisibility::SharedPtr STSM_CalculateVisibility::create(RenderContext* pRenderContext, const Dictionary& dict)
{
    return SharedPtr(new STSM_CalculateVisibility());
}

std::string STSM_CalculateVisibility::getDesc() { return kDesc; }

Dictionary STSM_CalculateVisibility::getScriptingDictionary()
{
    return Dictionary();
}

RenderPassReflection STSM_CalculateVisibility::reflect(const CompileData& compileData)
{
    // Define the required resources here
    RenderPassReflection reflector;
    reflector.addInput(kDepth, "Depth");
    reflector.addInput(kShadowMapSet, "ShadowMapSet").texture2D(0, 0, 0, 1, 0);
    reflector.addOutput(kVisibility, "Visibility").bindFlags(ResourceBindFlags::RenderTarget | Resource::BindFlags::ShaderResource).format(ResourceFormat::RGBA32Float).texture2D(0, 0);
    reflector.addOutput(kDebug, "Debug").bindFlags(ResourceBindFlags::RenderTarget | Resource::BindFlags::ShaderResource).format(ResourceFormat::RGBA32Float).texture2D(0, 0);
    reflector.addOutput(kLightUv, "LightUv").bindFlags(ResourceBindFlags::RenderTarget | Resource::BindFlags::ShaderResource).format(ResourceFormat::RG32Float).texture2D(0, 0).flags(RenderPassReflection::Field::Flags::Optional);
    return reflector;
}

void STSM_CalculateVisibility::execute(RenderContext* vRenderContext, const RenderData& vRenderData)
{
    if (!mpScene) return;

    if (!__loadPassInternalData(vRenderData)) return;

    const auto pCamera = mpScene->getCamera().get();
    const auto& pVisibility = vRenderData[kVisibility]->asTexture();
    const auto& pShadowMapSet = vRenderData[kShadowMapSet]->asTexture();
    const auto& pDepth = vRenderData[kDepth]->asTexture();
    const auto& pDebug = vRenderData[kDebug]->asTexture();
    const auto& pLightUv = vRenderData[kLightUv]->asTexture();

    mNumShadowMap = pShadowMapSet->getArraySize();

    mVisibilityPass.pFbo->attachColorTarget(pVisibility, 0);
    mVisibilityPass.pFbo->attachColorTarget(pLightUv, 1);
    mVisibilityPass.pFbo->attachColorTarget(pDebug, 2);
    vRenderContext->clearFbo(mVisibilityPass.pFbo.get(), float4(1, 0, 0, 0), 1, 0, FboAttachmentType::All);

    mVisibilityPass.pPass["gCompareSampler"] = mVisibilityPass.pLinearCmpSampler;
    mVisibilityPass.pPass["gShadowMapSet"] = pShadowMapSet;
    mVisibilityPass.pPass["gDepth"] = pDepth;
    mVisibilityPass.pPass["PerFrameCB"]["gShadowMapData"].setBlob(mVisibilityPass.ShadowMapData);
    mVisibilityPass.pPass["PerFrameCB"]["gDepthBias"] = mVContronls.DepthBias;
    mVisibilityPass.pPass["PerFrameCB"]["gSelectNum"] = mVContronls.SelectNum;
    mVisibilityPass.pPass["PerFrameCB"]["gRandomSelection"] = mVContronls.RandomSelection;
    mVisibilityPass.pPass["PerFrameCB"]["gTime"] = mVisibilityPass.Time;
    mVisibilityPass.pPass["PerFrameCB"]["gInvViewProj"] = pCamera->getInvViewProjMatrix();
    mVisibilityPass.pPass["PerFrameCB"]["gScreenDimension"] = uint2(mVisibilityPass.pFbo->getWidth(), mVisibilityPass.pFbo->getHeight());
    mVisibilityPass.pPass["PerFrameCB"]["gPcfRadius"] = mVContronls.PcfRadius;

    Camera::SharedConstPtr pC = mpScene->getCamera();
    Light::SharedConstPtr pL = mpScene->getLight(0);
    Helper::ShadowVPHelper SVPH(pC,pL,1);
    //mVisibilityPass.pPass["PerFrameCB"]["gCenterSVP"] = ;

    const std::string EventName = "Render Visibility Buffer";
    Profiler::instance().startEvent(EventName);
    mVisibilityPass.pPass->execute(vRenderContext, mVisibilityPass.pFbo); // Render visibility buffer
    Profiler::instance().endEvent(EventName);

    mVisibilityPass.Time += 100.f;
}

void STSM_CalculateVisibility::renderUI(Gui::Widgets& widget)
{
    widget.var("PCF Radius", mVContronls.PcfRadius, 0, 10, 1);
    widget.var("Depth Bias", mVContronls.DepthBias, 0.0f, 0.1f, 0.00001f);
    widget.dropdown("Map Num", mMapNumList, mVContronls.SelectNum);
    widget.checkbox("Randomly Select Shadow Map", mVContronls.RandomSelection);
}

void STSM_CalculateVisibility::setScene(RenderContext* pRenderContext, const Scene::SharedPtr& pScene)
{
    mpScene = pScene;
    _ASSERTE(mpScene);
}

bool STSM_CalculateVisibility::__loadPassInternalData(const RenderData& vRenderData)
{
    const InternalDictionary& Dict = vRenderData.getDictionary();
    InternalDictionary& rDict = vRenderData.getDictionary();
    if (!Dict.keyExists("ShadowMapData")) return false;
    mVisibilityPass.ShadowMapData = Dict["ShadowMapData"];
    rDict["foo"] = 1.23f;
    return true;
}
