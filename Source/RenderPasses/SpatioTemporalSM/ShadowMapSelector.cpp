#include "ShadowMapSelector.h"
#include "ShadowMapDefines.h"

namespace
{
    const char kDesc[] = "Shadow Map Generation Selection pass";

    // input
    const std::string kRasterize = "Rasterize";
    const std::string kViewWarp = "ViewWarp";

    // output
    const std::string kShadowMap = "ShadowMap";
}

Gui::DropdownList STSM_ShadowMapSelector::mGenerationTypeList =
{
    Gui::DropdownValue{ int(EShadowMapGenerationType::RASTERIZE), __toString(EShadowMapGenerationType::RASTERIZE) },
    Gui::DropdownValue{ int(EShadowMapGenerationType::VIEW_WARP), __toString(EShadowMapGenerationType::VIEW_WARP) }
};

STSM_ShadowMapSelector::STSM_ShadowMapSelector()
{
}

STSM_ShadowMapSelector::SharedPtr STSM_ShadowMapSelector::create(RenderContext* pRenderContext, const Dictionary& dict)
{
    return SharedPtr(new STSM_ShadowMapSelector());
}

std::string STSM_ShadowMapSelector::getDesc() { return kDesc; }

Dictionary STSM_ShadowMapSelector::getScriptingDictionary()
{
    return Dictionary();
}

RenderPassReflection STSM_ShadowMapSelector::reflect(const CompileData& compileData)
{
    // Define the required resources here
    RenderPassReflection reflector;
    reflector.addInput(kRasterize, "Rasterize").texture2D(0, 0, 0, 1, 0);
    reflector.addInput(kViewWarp, "ViewWarp").texture2D(0, 0, 0, 1, 0);
    reflector.addOutput(kShadowMap, "ShadowMap").bindFlags(Resource::BindFlags::RenderTarget | Resource::BindFlags::ShaderResource).format(gShadowMapDepthFormat).texture2D(gShadowMapSize.x, gShadowMapSize.y, 0, 1, gShadowMapNumPerFrame);;
    return reflector;
}

void STSM_ShadowMapSelector::compile(RenderContext* pContext, const CompileData& compileData)
{
}

void STSM_ShadowMapSelector::execute(RenderContext* pRenderContext, const RenderData& renderData)
{
    const auto& pRasterize = renderData[kRasterize]->asTexture();
    const auto& pViewWarp = renderData[kViewWarp]->asTexture();
    const auto& pShadowMap = renderData[kShadowMap]->asTexture();

    switch (mVContronls.GenerationType)
    {
    case EShadowMapGenerationType::RASTERIZE:
        __copyTextureArray(pRenderContext, pRasterize, pShadowMap);
        break;
    case EShadowMapGenerationType::VIEW_WARP:
        __copyTextureArray(pRenderContext, pViewWarp, pShadowMap);
        break;
    default: should_not_get_here();
    }

    // write chosen pass to dictionary
    InternalDictionary& Dict = renderData.getDictionary();
    Dict["ChosenShadowMapPass"] = mVContronls.GenerationType;
}

void STSM_ShadowMapSelector::setScene(RenderContext* pRenderContext, const Scene::SharedPtr& pScene)
{
    mpScene = pScene;
    mSceneTriangleNum = 0;
    for (uint i = 0; i < mpScene->getMeshCount(); ++i)
    {
        mSceneTriangleNum += mpScene->getMesh(i).getTriangleCount();
    }

    __updateGenerationType();
}

void STSM_ShadowMapSelector::renderUI(Gui::Widgets& widget)
{
    widget.text("Current Method: " + __toString(mVContronls.GenerationType));
    widget.text("Current Scene Triangle Number: " + std::to_string(mSceneTriangleNum));
    if (widget.checkbox("Auto Selection", mVContronls.EnableAutoSelection))
    {
        __updateGenerationType();
    }

    if (mVContronls.EnableAutoSelection)
    {
        if (widget.var("Triangle Number Threshold", mVContronls.TriangleNumThreshold, 10000u, 20000000u, 10000u))
        {
            __updateGenerationType();
        }
        widget.tooltip("When scene is complex, we prefer using view warp method.\nSo when triangle number exceeds this threshold, rasterize method will switch to view warp method", true);
    }
    else
    {
        uint Index = (uint)mVContronls.GenerationType;
        widget.dropdown("Method", mGenerationTypeList, Index);
        mVContronls.GenerationType = (EShadowMapGenerationType)Index;
    }
}

void STSM_ShadowMapSelector::__updateGenerationType()
{
    if (!mpScene) return;
    if (!mVContronls.EnableAutoSelection) return;

    if (mSceneTriangleNum > mVContronls.TriangleNumThreshold)
        mVContronls.GenerationType = EShadowMapGenerationType::VIEW_WARP;
    else
        mVContronls.GenerationType = EShadowMapGenerationType::RASTERIZE;
}

void STSM_ShadowMapSelector::__copyTextureArray(RenderContext* vRenderContext, std::shared_ptr<Texture> vSrc, std::shared_ptr<Texture> vDst)
{
    _ASSERTE(vRenderContext && vSrc && vDst);
    uint ArraySize = vSrc->getArraySize();
    _ASSERTE(ArraySize == vDst->getArraySize());

    for (uint i = 0; i < ArraySize; ++i)
    {
        vRenderContext->blit(vSrc->getSRV(0, 1, i, 1), vDst->getRTV(0, i, 1));
    }
}

std::string STSM_ShadowMapSelector::__toString(EShadowMapGenerationType vType)
{
    switch (vType)
    {
    case EShadowMapGenerationType::RASTERIZE: return "Rasterize";
    case EShadowMapGenerationType::VIEW_WARP: return "View Warp";
    default: should_not_get_here(); return "";
    }
}
