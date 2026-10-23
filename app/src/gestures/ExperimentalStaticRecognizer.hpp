#pragma once

#include "gestures/HandFeatures.hpp"

#include <array>

namespace vc {

// Experimental, deterministic landmark-based recognition. The output classes
// live outside the five-class ONNX contract; no Python or retraining needed.
inline constexpr std::size_t kExperimentalStaticCount = kExperimentalStaticGestureCount;
using ExperimentalStaticScores = std::array<float, kExperimentalStaticCount>;

ExperimentalStaticScores experimentalStaticScores(const GestureGeometry &geometry);
GestureClass experimentalStaticClass(std::size_t index);

} // namespace vc
