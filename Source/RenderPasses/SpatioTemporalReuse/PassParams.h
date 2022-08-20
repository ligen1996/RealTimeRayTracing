#pragma once
#include "Falcor.h"

namespace Helper
{
    bool savePassParams(const pybind11::dict& vDict);
    bool parsePassParamsFile(std::string vFileName, pybind11::dict& voDict);
    bool openPassParams(pybind11::dict& voDict);
}
#pragma once
