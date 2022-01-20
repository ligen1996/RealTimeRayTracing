#pragma once
#include "Falcor.h"
#include "Utils/Sampling/SampleGenerator.h"

using namespace Falcor;

class CSampleGenerator
{
public:
    enum class ESamplePattern : uint
    {
        Center,
        DirectX,
        Halton,
        Stratitied,
    };//todo:may add more random pattern

    void init(ESamplePattern vPattern, uint vSampleNum);
    float2 getNextSample(float vComponentRange = 1.0f);

private:
    static CPUSampleGenerator::SharedPtr __createSamplePattern(ESamplePattern vType, uint vSampleNum);
    CPUSampleGenerator::SharedPtr m_pSampleGenerator;
};

