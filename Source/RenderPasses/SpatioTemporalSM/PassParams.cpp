#pragma once
#include "PassParams.h"

using namespace Falcor;

bool Helper::savePassParams(const pybind11::dict& vDict)
{
    FileDialogFilterVec Filters{ { "json", "JSON" } };
    std::string FileName;
    if (saveFileDialog(Filters, FileName))
    {
        pybind11::module json = pybind11::module::import("json");
        pybind11::object dumps = json.attr("dumps");
        auto Data = pybind11::cast<std::string>(dumps(vDict, "indent"_a = 2));
        std::ofstream File(FileName);
        File << Data;
        File.close();
        return true;
    }
    return false;
}

bool Helper::loadPassParams(pybind11::dict& voDict)
{
    FileDialogFilterVec Filters{ { "json", "JSON" } };
    std::string FileName;
    if (openFileDialog(Filters, FileName))
    {
        pybind11::module json = pybind11::module::import("json");
        pybind11::object loads = json.attr("loads");

        std::ifstream File(FileName);
        std::istreambuf_iterator<char> IterBegin(File), IterEnd;
        std::string Data(IterBegin, IterEnd);
        voDict = pybind11::cast<pybind11::dict>(loads(Data));
        return true;
    }
    return false;
}
