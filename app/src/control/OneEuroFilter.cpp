#include "control/OneEuroFilter.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace vc {

float OneEuroFilter::alpha(
    float cutoff,
    float dtSeconds)
{
    const float safeCutoff =
        std::max(cutoff, 0.001F);
    const float safeDt =
        std::max(dtSeconds, 0.000001F);

    const float tau =
        1.0F
        / (2.0F
           * std::numbers::pi_v<float>
           * safeCutoff);

    return 1.0F / (1.0F + tau / safeDt);
}

float OneEuroFilter::update(
    float value,
    std::int64_t timestampUs,
    float cutoff,
    float beta)
{
    if (!initialized_
        || timestampUs <= lastTimestampUs_
        || timestampUs - lastTimestampUs_ > 250000) {
        value_ = value;
        raw_ = value;
        derivative_ = 0.0F;
        lastTimestampUs_ = timestampUs;
        initialized_ = true;
        return value;
    }

    const float dt =
        static_cast<float>(
            timestampUs - lastTimestampUs_)
        / 1000000.0F;

    const float rawDerivative =
        (value - raw_) / dt;

    derivative_ +=
        alpha(1.0F, dt)
        * (rawDerivative - derivative_);

    const float adaptiveCutoff =
        std::max(
            0.001F,
            cutoff
                + std::max(beta, 0.0F)
                    * std::abs(derivative_));

    value_ +=
        alpha(adaptiveCutoff, dt)
        * (value - value_);

    raw_ = value;
    lastTimestampUs_ = timestampUs;
    return value_;
}

void OneEuroFilter::reset()
{
    value_ = 0.0F;
    derivative_ = 0.0F;
    raw_ = 0.0F;
    lastTimestampUs_ = 0;
    initialized_ = false;
}

} // namespace vc
