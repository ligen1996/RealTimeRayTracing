#pragma once

#ifndef SHADOW_MAP_DEFINES
#define SHADOW_MAP_DEFINES
#include "Falcor.h"

const Falcor::uint2 gShadowMapSize = Falcor::uint2(4096, 4096);
const Falcor::ResourceFormat gShadowMapDepthFormat = Falcor::ResourceFormat::R32Float;

#endif
