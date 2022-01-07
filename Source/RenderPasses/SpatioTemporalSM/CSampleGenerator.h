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

    void init(ESamplePattern vPattern, uint vSampleNum)
    {
        m_pSampleGenerator = __createSamplePattern(vPattern, vSampleNum);
    }

    float2 getNextSample(float vComponentRange = 1.0f)
    {
        _ASSERTE(m_pSampleGenerator);
        float2 Sample = m_pSampleGenerator->next();
        Sample *= vComponentRange * 2.0f;
        return Sample;
    }

private:
    static CPUSampleGenerator::SharedPtr __createSamplePattern(ESamplePattern vType, uint vSampleNum)
    {
        switch (vType)
        {
        case ESamplePattern::Center: return nullptr;
        case ESamplePattern::DirectX: return DxSamplePattern::create(vSampleNum);
        case ESamplePattern::Halton: return HaltonSamplePattern::create(vSampleNum);
        case ESamplePattern::Stratitied: return StratifiedSamplePattern::create(vSampleNum);
        default:
            should_not_get_here();
            return nullptr;
        }
    }

    CPUSampleGenerator::SharedPtr m_pSampleGenerator;
};

