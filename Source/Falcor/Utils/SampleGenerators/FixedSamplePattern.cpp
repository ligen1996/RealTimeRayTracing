#include "stdafx.h"
#include "FixedSamplePattern.h"

namespace Falcor
{
    CFixedSamplePattern::SharedPtr CFixedSamplePattern::create(uint32_t sampleCount)
    {
        return SharedPtr(new CFixedSamplePattern(sampleCount));
    }

    CFixedSamplePattern::CFixedSamplePattern(uint32_t sampleCount)
    {
        // Clamp sampleCount to a reasonable number so the permutation table doesn't get too big.
        if (sampleCount < 1) logWarning("CFixedSamplePattern() requires sampleCount > 0. Using one sample.");
        else if (sampleCount > 1024) logWarning("CFixedSamplePattern() requires sampleCount <= 1024. Using 1024 samples.");
        sampleCount = std::clamp(sampleCount, 1u, 1024u);

        auto dist = std::uniform_real_distribution<float>();
        auto u = [&]() { return dist(mRng); };
        mList.resize(sampleCount);
        for (uint32_t i = 0; i < sampleCount; i++)
            mList[i] = float2(u(), u()) - 0.5f;
    }

    float2 CFixedSamplePattern::next()
    {
        float2 Res = mList[mCurSample];
        mCurSample = (mCurSample + 1) % getSampleCount();
        return Res;
    }
}
