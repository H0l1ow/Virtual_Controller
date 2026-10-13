#pragma once

#include "gestures/GestureTypes.hpp"

#include <algorithm>
#include <array>

namespace vc {

// -1 inherits the existing global setting. Profile JSON and the ONNX input
// feature schema are not affected by per-class recognition thresholds.
struct GestureThresholds {
    int fallbackPercent{80};
    std::array<int, kGestureClassCount> overrides{{-1, -1, -1, -1, -1}};

    int forClass(GestureClass gesture) const
    {
        const auto index = gestureIndex(gesture);
        if (index == 0 || index >= overrides.size())
            return std::clamp(fallbackPercent, 0, 100);
        const int custom = overrides[index];
        return std::clamp(custom < 0 ? fallbackPercent : custom, 0, 100);
    }
};

} // namespace vc
