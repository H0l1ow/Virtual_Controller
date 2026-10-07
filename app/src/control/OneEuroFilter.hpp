#pragma once

#include <cstdint>

namespace vc {

// Small scalar One Euro filter used by the continuous-control layer. The
// filter deliberately resets after a long sample gap so reacquiring a hand
// cannot interpolate from stale history.
class OneEuroFilter final
{
public:
    float update(
        float value,
        std::int64_t timestampUs,
        float cutoff,
        float beta);

    void reset();

private:
    static float alpha(
        float cutoff,
        float dtSeconds);

    float value_{};
    float derivative_{};
    float raw_{};
    std::int64_t lastTimestampUs_{};
    bool initialized_{};
};

} // namespace vc
