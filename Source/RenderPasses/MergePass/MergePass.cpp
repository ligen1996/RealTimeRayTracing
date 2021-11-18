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
#include "MergePass.h"


namespace
{
    const char kDesc[] = "Merge serval Texures into one";

    const std::string kColorIn = "ColorIn";
    const std::string kVisibilityIn = "VisibilityIn";
    const std::string kColorOut = "MergedColor";

    const std::string kShaderFileName = "RenderPasses/MergePass/MergePass.ps.slang";
}

// Don't remove this. it's required for hot-reload to function properly
extern "C" __declspec(dllexport) const char* getProjDir()
{
    return PROJECT_DIR;
}

extern "C" __declspec(dllexport) void getPasses(Falcor::RenderPassLibrary& lib)
{
    lib.registerClass("MergePass", kDesc, MergePass::create);
}

MergePass::SharedPtr MergePass::create(RenderContext* pRenderContext, const Dictionary& dict)
{
    SharedPtr pPass = SharedPtr(new MergePass);
    return pPass;
}

std::string MergePass::getDesc() { return kDesc; }

Dictionary MergePass::getScriptingDictionary()
{
    return Dictionary();
}

RenderPassReflection MergePass::reflect(const CompileData& compileData)
{
    // Define the required resources here
    RenderPassReflection reflector;
    reflector.addInput(kColorIn, "Input Color");
    reflector.addInput(kVisibilityIn, "Input Visibility");
    reflector.addOutput(kColorOut, "Output Merged Color");
    return reflector;
}

void MergePass::execute(RenderContext* pRenderContext, const RenderData& renderData)
{
    // renderData holds the requested resources
    // auto& pTexture = renderData["src"]->asTexture();

    const auto& pColorIn = renderData[kColorIn]->asTexture();
    const auto& pVisibility = renderData[kVisibilityIn]->asTexture();
    const auto& pColorOut = renderData[kColorOut]->asTexture();

    mpFbo->attachColorTarget(pColorOut, 0);

    mpPass["gTexColorIn"] = pColorIn;
    mpPass["gTexVisibilityIn"] = pVisibility;
    
    mpPass->execute(pRenderContext, mpFbo);
}

void MergePass::renderUI(Gui::Widgets& widget)
{

}

MergePass::MergePass()
{
    mpPass = FullScreenPass::create(kShaderFileName);
    mpFbo = Fbo::create();
}
