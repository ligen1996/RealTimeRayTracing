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
#include "MultiViewShadowMapViewWarp.h"
#include "MultiViewShadowMapRasterize.h"
#include "ShadowMapSelector.h"
#include "TemporalReuse.h"
#include "ReuseFactorEstimation.h"
#include "CalculateVisibility.h"
#include "BilateralFilter.h"

static const char DescShadowMapViewWarp[] = "Multi View Shadow Map by View Warp";
static const char DescShadowMapRasterize[] = "Multi View Shadow Map by Rasterization";
static const char DescShadowMapSelector[] = "Shadow Map Generation Method Selection";
static const char DescVisibility[] = "Calculate Visibility";
static const char DescReuseFactorEstimation[] = "Reuse Factor Estimation";
static const char DescReuse[] = "Temporal Reuse";
static const char DescBilateralFilter[] = "Bilateral Filter";

extern "C" __declspec(dllexport) const char* getProjDir()
{
    return PROJECT_DIR;
}

extern "C" __declspec(dllexport) void getPasses(Falcor::RenderPassLibrary& lib)
{
    //lib.registerClass("STSM_MultiViewShadowMapViewWarp", DescShadowMapViewWarp, STSM_MultiViewShadowMapViewWarp::create);
    lib.registerClass("STSM_MultiViewShadowMapRasterize", DescShadowMapRasterize, STSM_MultiViewShadowMapRasterize::create);
    //lib.registerClass("STSM_ShadowMapSelector", DescShadowMapSelector, STSM_ShadowMapSelector::create);
    lib.registerClass("STSM_CalculateVisibility", DescVisibility, STSM_CalculateVisibility::create);
    lib.registerClass("STSM_ReuseFactorEstimation", DescReuseFactorEstimation, STSM_ReuseFactorEstimation::create);
    lib.registerClass("STSM_TemporalReuse", DescReuse, STSM_TemporalReuse::create);
    lib.registerClass("STSM_BilateralFilter", DescBilateralFilter, STSM_BilateralFilter::create);
}
