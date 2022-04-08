#include "CSampleGenerator.h"
#include "Utils/SampleGenerators/FixedSamplePattern.h"

void CSampleGenerator::init(ESamplePattern vPattern, uint vSampleNum)
{
    m_pSampleGenerator = __createSamplePattern(vPattern, vSampleNum);
}

float2 CSampleGenerator::getNextSample(float vComponentRange)
{
    _ASSERTE(m_pSampleGenerator);
    float2 Sample = m_pSampleGenerator->next();
    Sample *= vComponentRange * 2.0f;
    return Sample;
}

CPUSampleGenerator::SharedPtr CSampleGenerator::__createSamplePattern(ESamplePattern vType, uint vSampleNum)
{
    switch (vType)
    {
    case ESamplePattern::Center: return nullptr;
    case ESamplePattern::DirectX: return DxSamplePattern::create(vSampleNum);
    case ESamplePattern::Halton: return HaltonSamplePattern::create(vSampleNum);
    case ESamplePattern::Stratitied: return StratifiedSamplePattern::create(vSampleNum);
    case ESamplePattern::Fixed: return CFixedSamplePattern::create(vSampleNum);
    default:
        should_not_get_here();
        return nullptr;
    }
}
