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

//#include "glm/gtc/integer.hpp"
//#include "glm/gtx/euler_angles.hpp"

#include "LTCLight.h"
#include <cstring>


namespace
{
    const char kDesc[] = "Implement LTC polygonal light.";

    const std::string kProgramFile = "RenderPasses/LTCLight/LTCLight.ps.slang";
    const std::string kDebugProgramFile = "RenderPasses/LTCLight/DrawLight.slang";
    const std::string shaderModel = "6_2";

    const std::string kLTCMatrix = "gLTCMatrix";
    const std::string kLTCMagnitue = "gLTCMagnitue";
    const std::string kLTCLightColor = "gLightColor";
    const std::string kLTCMatrixFile = "../Data/Texture/ltc_mat.dds";
    const std::string kLTCMagnitueFile = "../Data/Texture/ltc_amp.dds";

    const ChannelList kInChannels =
    {
        { "PosW"        , "gPosW"       ,  "World position"     , false , ResourceFormat::RGBA32Float},
        { "NormalW"     , "gNormalW"    ,  "World normal"       , false , ResourceFormat::RGBA32Float},
        { "Tangent"     , "gTangent"    ,  "Tangent Vector"     , false , ResourceFormat::RGBA32Float},
        { "Roughness"   , "gRoughness"  ,  "Roughness"          , true  , ResourceFormat::RGBA8Unorm},
        { "DiffuseOpacity", "gDiffuseOpacity",  "diffuse color and opacity",    true , ResourceFormat::RGBA32Float },
        { "SpecRough"   , "gSpecRough"  ,       "specular color and roughness", true , ResourceFormat::RGBA32Float },
        { "Visibility"  , "gVisibility" ,  "Visibility"         , true  , ResourceFormat::RGBA32Float},
        { "SkyBox"      , "gSkyBox"     ,  "Sky Box"            , true  , ResourceFormat::RGBA32Float},
        //{ "LightColor", "gLightColor", "Color in each point of light", true /* optional */, ResourceFormat::RGBA32Float},
    };

    const char kDepth[] = "Depth";
    const char kColor[] = "Color";
    const char kColorDesc[] = "LTC polygonal light shading result.";
}

// Don't remove this. it's required for hot-reload to function properly
extern "C" __declspec(dllexport) const char* getProjDir()
{
    return PROJECT_DIR;
}

extern "C" __declspec(dllexport) void getPasses(Falcor::RenderPassLibrary& lib)
{
    lib.registerClass("LTCLight", kDesc, LTCLight::create);
}

LTCLight::SharedPtr LTCLight::create(RenderContext* pRenderContext, const Dictionary& dict)
{
    SharedPtr pPass = SharedPtr(new LTCLight);
    return pPass;
}

std::string LTCLight::getDesc() { return kDesc; }

Dictionary LTCLight::getScriptingDictionary()
{
    return Dictionary();
}

LTCLight::LTCLight()
{
    mpPass = FullScreenPass::create(kProgramFile);
    mpFbo = Fbo::create();

    // init Textures
    mpLTCMatrixTex = Texture::createFromFile(kLTCMatrixFile, false, false);
    mpLTCMagnitueTex = Texture::createFromFile(kLTCMagnitueFile , false, false);
    mpLTCLightColorTex = __generateLightColorTex();

    Sampler::Desc Desc;
    Desc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Point);
    Desc.setAddressingMode(Sampler::AddressMode::Clamp, Sampler::AddressMode::Clamp, Sampler::AddressMode::Clamp);
    mpSampler = Sampler::create(Desc);

    __initPassData();

    // Light debug only, maybe abstracted out!
    __initDebugDrawerResources();
}

RenderPassReflection LTCLight::reflect(const CompileData& compileData)
{
    // Define the required resources here
    RenderPassReflection reflector;
    reflector.addInput(kDepth, "Depth for draw light.").format(ResourceFormat::D32Float);
    addRenderPassInputs(reflector, kInChannels, ResourceBindFlags::ShaderResource);
    reflector.addOutput(kColor, kColorDesc).format(ResourceFormat::Unknown).texture2D(0, 0, 0);
    //reflector.addInputOutput(kColor, kColorDesc).format(ResourceFormat::Unknown).texture2D(0, 0, 0);
    return reflector;
}

void LTCLight::execute(RenderContext* pRenderContext, const RenderData& renderData)
{
    if (!mpScene)
    {
        return;
    }

    if (mUseTextureLight)
    {
        mpPass->addDefine("USE_TEXTURE_LIGHT");
        mDebugDrawerResource.mpProgram->addDefine("USE_TEXTURE_LIGHT");
    }
    else
    {
        mpPass->removeDefine("USE_TEXTURE_LIGHT");
        mDebugDrawerResource.mpProgram->removeDefine("USE_TEXTURE_LIGHT");
    }

    // attach and clear output textures to fbo
    const auto& pColorTex = renderData[kColor]->asTexture();

    mpFbo->attachColorTarget(pColorTex, 0);

    // set all textures
    for (const auto& channel : kInChannels)
    {
        auto pTex = renderData[channel.name]->asTexture();
        if (pTex != nullptr)
        {
            mpPass->addDefine("has_" + channel.texname);
        }
        else
        {
            mpPass->removeDefine("has_" + channel.texname);
        }
        mpPass[channel.texname] = pTex;
    }
    mpPass->getVars()[kLTCMatrix] = mpLTCMatrixTex;
    mpPass->getVars()[kLTCMagnitue] = mpLTCMagnitueTex;
    mpPass->getVars()[kLTCLightColor] = mpLTCLightColorTex;

    // update camera data
    auto pCam = mpScene->getCamera();
    mPassData.CamPosW = float4(pCam->getPosition(),1.0);
    mPassData.MatV = pCam->getViewMatrix();
    mPassData.MatP = pCam->getProjMatrix();

    // update light vertexes
    __updateRectLightProperties();

    GraphicsVars::SharedPtr pVar = mpPass->getVars();
    UniformShaderVarOffset Offset = pVar->getParameterBlock("PerFrameCB")->getVariableOffset("g");
    pVar["PerFrameCB"][Offset].setBlob(mPassData);
    pVar["gSampler"] = mpSampler;

    __prepareEnvMap(pRenderContext);

    mpPass->execute(pRenderContext, mpFbo);

    // Light debug only, maybe abstracted out!
    auto pDepth = renderData[kDepth]->asTexture();
    mpFbo->attachDepthStencilTarget(pDepth);
    __drawLightDebug(pRenderContext);
}

void LTCLight::renderUI(Gui::Widgets& widget)
{
    static bool TwoSide = (mPassData.TwoSide>0.5);
    widget.checkbox("TwoSide", TwoSide);
    if (TwoSide)
    {
        mPassData.TwoSide = 1.;
        RasterizerState::Desc wireframeDesc;
        wireframeDesc.setCullMode(RasterizerState::CullMode::None);
        mDebugDrawerResource.mpRasterState = RasterizerState::create(wireframeDesc);
        mDebugDrawerResource.mpGraphicsState->setRasterizerState(mDebugDrawerResource.mpRasterState);
    }
    else
    {
        mPassData.TwoSide = 0.;
        RasterizerState::Desc wireframeDesc;
        wireframeDesc.setCullMode(RasterizerState::CullMode::Back);
        mDebugDrawerResource.mpRasterState = RasterizerState::create(wireframeDesc);
        mDebugDrawerResource.mpGraphicsState->setRasterizerState(mDebugDrawerResource.mpRasterState);
    }

    widget.checkbox("UseTextureLight", mUseTextureLight);
    
    widget.var("Roughness", mPassData.Roughness, 0.0f, 1.0f, 0.02f);
    //widget.var("Intensity", mpPassData.Intensity, 0.0f, 100.0f, 0.1f);
    widget.rgbaColor("Diffuse Color", mPassData.DiffuseColor);
    widget.rgbaColor("Specular Color", mPassData.SpecularColor);
    if (widget.rgbaColor("Light Tint", mPassData.LightTint))
    {
        mpLightDebugDrawer->setColor(mPassData.LightTint);
    }

    /*widget.var("Rotation XYZ", mEnvLightData.EularRotation, -360.f, 360.f, 0.5f);
    widget.var("EnvIntensity", mEnvLightData.Intensity, 0.f, 20.f, 0.02f);*/
}

void LTCLight::setScene(RenderContext* pRenderContext, const Scene::SharedPtr& pScene)
{
    assert(pScene);
    mpScene = pScene;
    
    if (!(mpLight = std::dynamic_pointer_cast<RectLight>(mpScene->getLightByName("RectLight"))))
    {
        mpLight = std::dynamic_pointer_cast<RectLight>(mpScene && mpScene->getLightCount() ? mpScene->getLight(0) : nullptr);
    }
    assert(mpLight);

    mpCam = mpScene->getCamera();
}

void LTCLight::__updateRectLightProperties()
{
    assert(mpLight);
    mPassData.LightPolygonPoints[3] = (float4(mpLight->getPosByUv(float2(-1, -1)), 1.0));
    mPassData.LightPolygonPoints[2] = (float4(mpLight->getPosByUv(float2(1, -1)), 1.0));
    mPassData.LightPolygonPoints[1] = (float4(mpLight->getPosByUv(float2(1, 1)), 1.0));
    mPassData.LightPolygonPoints[0] = (float4(mpLight->getPosByUv(float2(-1, 1)), 1.0));

    mPassData.Intensity = mpLight->getIntensity().x;
}

void LTCLight::__initPassData()
{
    mPassData.DiffuseColor = float4(1.);
    mPassData.SpecularColor = float4(1.);
    mPassData.Roughness = 1.f;
    mPassData.Intensity = 1.0;
    mPassData.TwoSide = 0.0;
    mPassData.Padding;

    mpPass->addDefine("USE_TEXTURE_LIGHT");
}

Texture::SharedPtr LTCLight::__generateLightColorTex()
{
    std::string fullpath;
    uint8_t* Colors = nullptr;
    ResourceFormat texFormat;
    uint32_t width,height;
    int ct = 0;

    for (size_t i = 0; i < 8; i++)
    {
        std::string filename = "../Data/Texture/";
        filename += std::to_string(i) + ".png";
        if (findFileInDataDirectories(filename, fullpath) == false)
        {
            logWarning("Error when loading image file. Can't find image file '" + filename + "'");
            if (ct > 0) delete[] Colors;
            return nullptr;
        }

        Bitmap::UniqueConstPtr pBitmap = Bitmap::createFromFile(fullpath, true);

        if (pBitmap)//
        {
            if (ct == 0)
            {
                texFormat = pBitmap->getFormat();
                width = pBitmap->getWidth();
                height = pBitmap->getHeight();
                
                Colors = new uint8_t[8*pBitmap->getSize()];
            }
            else
            {
                if (texFormat != pBitmap->getFormat() || width != pBitmap->getWidth() || height != pBitmap->getHeight())
                {
                    logWarning("Light Color" + std::to_string(i) +  " Don't match!");
                }
            }
        }
        else
        {
            logWarning("Error when loading image file. Can't find image file '" + filename + "'");
            return nullptr;
        }

        auto Data = pBitmap->getData();
        memcpy(Colors + ct * pBitmap->getSize(), Data, pBitmap->getSize());
        ct++;
        //Colors[ct++] = pBitmap->getData();
    }

    if (ct != 8)
    {
        logWarning("No enough Light Color texture");
        if (Colors) delete[] Colors;
        return nullptr;
    }

    Texture::SharedPtr pTex;
    pTex = Texture::create2D(width, height,  texFormat, 8, 1, Colors , ResourceBindFlags::ShaderResource);
    //pTex = Texture::create3D(width, height, 8, texFormat, 1, Colors , ResourceBindFlags::ShaderResource);

    if (pTex != nullptr)
    {
        pTex->setSourceFilename(fullpath);
    }
    if (Colors) delete[] Colors;
    return pTex;
}

void LTCLight::__initDebugDrawerResources()
{
    mpLightDebugDrawer = TriangleDebugDrawer::create();
    mpLightDebugDrawer->clear();
    mpLightDebugDrawer->setColor(float3(1));

    //program
    Program::Desc desc;
    desc.addShaderLibrary(kDebugProgramFile).vsEntry("vsMain").psEntry("psMain");
    desc.setShaderModel(shaderModel);
    mDebugDrawerResource.mpProgram = GraphicsProgram::create(desc);
    mDebugDrawerResource.mpProgram->addDefine("USE_TEXTURE_LIGHT");
    mDebugDrawerResource.mpVars = GraphicsVars::create(mDebugDrawerResource.mpProgram.get());

    //state
    DepthStencilState::Desc DepthStateDesc;
    DepthStateDesc.setDepthEnabled(true);
    DepthStateDesc.setDepthWriteMask(true);
    DepthStateDesc.setDepthFunc(DepthStencilState::Func::Less);
    DepthStencilState::SharedPtr pDepthState = DepthStencilState::create(DepthStateDesc);

    RasterizerState::Desc wireframeDesc;
    wireframeDesc.setCullMode(RasterizerState::CullMode::None);
    mDebugDrawerResource.mpRasterState = RasterizerState::create(wireframeDesc);

    mDebugDrawerResource.mpGraphicsState = GraphicsState::create();
    mDebugDrawerResource.mpGraphicsState->setProgram(mDebugDrawerResource.mpProgram);
    mDebugDrawerResource.mpGraphicsState->setRasterizerState(mDebugDrawerResource.mpRasterState);
    mDebugDrawerResource.mpGraphicsState->setDepthStencilState(pDepthState);

    //quad presentation
    DebugDrawer::Quad quad;
    quad[0] = float3(-1, -1, 0); //LL
    quad[1] = float3(-1, 1, 0);  //LU
    quad[2] = float3(1, 1, 0);   //RU
    quad[3] = float3(1, -1, 0);  //RL

    mpLightDebugDrawer->addPoint(quad[3], float2(1, 0));
    mpLightDebugDrawer->addPoint(quad[1], float2(0, 1));
    mpLightDebugDrawer->addPoint(quad[0], float2(0, 0));
    mpLightDebugDrawer->addPoint(quad[2], float2(1, 1));
    mpLightDebugDrawer->addPoint(quad[1], float2(0, 1));
    mpLightDebugDrawer->addPoint(quad[3], float2(1, 0));
}

void LTCLight::__drawLightDebug(RenderContext* vRenderContext)
{
    mDebugDrawerResource.mpGraphicsState->setFbo(mpFbo);
    const auto pCamera = mpScene->getCamera().get();
    mDebugDrawerResource.mpVars["PerFrameCB"]["gMatLightLocal2PosW"] = mpLight->getData().transMat * glm::scale(glm::mat4(),float3(mpLight->getSize() * float2(0.5),1));
    mDebugDrawerResource.mpVars["PerFrameCB"]["gMatCamVP"] = pCamera->getViewProjMatrix() ;
    mDebugDrawerResource.mpVars["PerFrameCB"]["gLightColor"] = mPassData.LightTint;
    mDebugDrawerResource.mpVars["gLightTex"] = Texture::createFromFile("../Data/Texture/1.png", false, false);
    mDebugDrawerResource.mpVars["gSampler"] = mpSampler;
    mpLightDebugDrawer->render(vRenderContext, mDebugDrawerResource.mpGraphicsState.get(), mDebugDrawerResource.mpVars.get(), pCamera);
}

float3x4 XYZtoMat(float3 degreesXYZ)
{
    /*auto transform = glm::eulerAngleXYZ(glm::radians(degreesXYZ.x), glm::radians(degreesXYZ.y), glm::radians(degreesXYZ.z));

    return static_cast<float3x4>(transform);*/
    float3x4 transform;
    return transform;
}

void LTCLight::__prepareEnvMap(RenderContext* vRenderContext)
{
    if (!mpScene) return;
    // Update env map lighting
    auto& pVars = mpPass->getVars();
    auto& pState = mpPass->getState();
    auto& pEnvMap = mpScene->getEnvMap();

    if (pEnvMap)
    {
        if ((!mpEnvMapLighting || mpEnvMapLighting->getEnvMap() != pEnvMap))
        {
            mpEnvMapLighting = EnvMapLighting::create(vRenderContext, pEnvMap);
            float3x4 InvTransform = pEnvMap->getInvTransform();
            mpEnvMapLighting->setShaderData(pVars["PerFrameCB"]["gEnvMapLighting"]);
            pState->getProgram()->addDefine("_USE_ENV_MAP");
        }
        pVars["PerFrameCB"]["gEnvMapLighting"]["gIntensity"] = pEnvMap->getIntensity();
        pVars["PerFrameCB"]["gEnvMapLighting"]["gInvTransform"] = pEnvMap->getInvTransform();
    }
    else if (!pEnvMap)
    {
        mpEnvMapLighting = nullptr;
        pState->getProgram()->removeDefine("_USE_ENV_MAP");
    }
}
