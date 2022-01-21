#pragma once

#ifndef SHADOW_MAP_DEFINES
#define SHADOW_MAP_DEFINES
#include "Falcor.h"

const Falcor::uint gRectLightSampleGridSize = 4;
const Falcor::uint gShadowMapNumPerFrame = gRectLightSampleGridSize * gRectLightSampleGridSize;
const Falcor::uint2 gShadowMapSize = Falcor::uint2(1024, 1024);
const Falcor::ResourceFormat gShadowMapDepthFormat = Falcor::ResourceFormat::R32Float;

#endif
