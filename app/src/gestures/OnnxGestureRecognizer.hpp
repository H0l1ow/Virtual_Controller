#pragma once

#include "gestures/GestureTypes.hpp"
#include "gestures/HandFeatures.hpp"

#include <QString>

#include <memory>
#include <vector>

namespace vc {

// Optional native ONNX Runtime classifier. The application always builds with
// the rule recognizer; this class becomes active only when VC_WITH_ONNX=ON and
// a compatible gesture_model.onnx + model_metadata.json are present.
class OnnxGestureRecognizer final
{
public:
    explicit OnnxGestureRecognizer(
        const QString &modelPath);

    ~OnnxGestureRecognizer();

    OnnxGestureRecognizer(
        const OnnxGestureRecognizer &) = delete;
    OnnxGestureRecognizer &operator=(
        const OnnxGestureRecognizer &) = delete;

    GestureScores infer(
        const std::vector<float> &tensor);

    int windowSize() const
    {
        return windowSize_;
    }

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    int windowSize_{16};
};

} // namespace vc
