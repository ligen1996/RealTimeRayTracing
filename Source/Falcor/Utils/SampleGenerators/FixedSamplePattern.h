#pragma once
#include "CPUSampleGenerator.h"
#include <random>

namespace Falcor
{
    class dlldecl CFixedSamplePattern : public CPUSampleGenerator
    {
    public:
        using SharedPtr = std::shared_ptr<CFixedSamplePattern>;

        virtual ~CFixedSamplePattern() = default;

        static SharedPtr create(uint32_t sampleCount = 1);
        virtual uint32_t getSampleCount() const override { return uint32_t(mList.size()); }
        virtual void reset(uint32_t startID = 0) override { mCurSample = startID; }
        virtual float2 next() override;

    protected:
        CFixedSamplePattern(uint32_t sampleCount);

        uint32_t mCurSample = 0;
        std::mt19937 mRng;
        std::vector<float2> mList;
    };
}
