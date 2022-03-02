#pragma once
#include "Falcor.h"

namespace Helper
{
    bool savePassParams(const pybind11::dict& vDict);
    bool loadPassParams(pybind11::dict& voDict);
}
