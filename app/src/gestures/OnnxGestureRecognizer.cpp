#include "gestures/OnnxGestureRecognizer.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <string>

#ifdef VC_WITH_ONNX
#include <onnxruntime_cxx_api.h>
#endif

namespace vc {

namespace {

QJsonObject readMetadata(
    const QString &modelPath)
{
    const QString metadataPath =
        QFileInfo(modelPath)
            .dir()
            .filePath(
                QStringLiteral("model_metadata.json"));

    QFile file(metadataPath);

    if (!file.open(QIODevice::ReadOnly)) {
        throw std::runtime_error(
            "Missing model_metadata.json next to gesture model");
    }

    const auto document =
        QJsonDocument::fromJson(file.readAll());

    if (!document.isObject()) {
        throw std::runtime_error(
            "Invalid gesture model metadata JSON");
    }

    return document.object();
}

void validateMetadata(
    const QJsonObject &metadata,
    int &windowSize)
{
    if (metadata.value(QStringLiteral("schemaVersion")).toInt() != 2
        || metadata.value(QStringLiteral("featureSchema")).toString()
            != QString::fromLatin1(kGestureFeatureSchema)
        || metadata.value(QStringLiteral("featureCount")).toInt()
            != static_cast<int>(kGestureFeatureCount)
        || metadata.value(QStringLiteral("sampleRate")).toInt()
            != kGestureSampleRate
        || metadata.value(QStringLiteral("inputName")).toString()
            != QStringLiteral("features")
        || metadata.value(QStringLiteral("outputName")).toString()
            != QStringLiteral("logits")
        || !metadata.value(QStringLiteral("normalizationEmbedded")).toBool()) {
        throw std::runtime_error(
            "Incompatible gesture model feature contract");
    }

    windowSize =
        metadata.value(
            QStringLiteral("windowSize"))
            .toInt();

    if (windowSize < 2
        || windowSize > 120) {
        throw std::runtime_error(
            "Invalid gesture model temporal window");
    }

    const auto labels =
        metadata.value(
            QStringLiteral("labels"))
            .toArray();

    if (labels.size()
        != static_cast<int>(kLegacyOnnxGestureClassCount)) {
        throw std::runtime_error(
            "Gesture model class count mismatch");
    }

    for (int index = 0;
         index < labels.size();
         ++index) {
        if (labels[index].toString()
            != QString::fromLatin1(
                kLegacyOnnxGestureCanonicalNames[
                    static_cast<std::size_t>(index)])) {
            throw std::runtime_error(
                "Gesture model label order mismatch");
        }
    }
}

} // namespace

struct OnnxGestureRecognizer::Impl {
#ifdef VC_WITH_ONNX
    Ort::Env environment{
        ORT_LOGGING_LEVEL_WARNING,
        "VirtualControllerGesture"
    };

    Ort::SessionOptions options;
    std::unique_ptr<Ort::Session> session;
#endif
};

OnnxGestureRecognizer::OnnxGestureRecognizer(
    const QString &modelPath)
    : impl_(std::make_unique<Impl>())
{
#ifndef VC_WITH_ONNX
    Q_UNUSED(modelPath);
    throw std::runtime_error(
        "This build has no ONNX Runtime. Configure VC_WITH_ONNX=ON.");
#else
    if (!QFileInfo::exists(modelPath)) {
        throw std::runtime_error(
            "gesture_model.onnx does not exist");
    }

    const auto metadata =
        readMetadata(modelPath);
    validateMetadata(
        metadata,
        windowSize_);

    impl_->options.SetIntraOpNumThreads(1);
    impl_->options.SetInterOpNumThreads(1);
    impl_->options.SetGraphOptimizationLevel(
        GraphOptimizationLevel::ORT_ENABLE_ALL);

#ifdef Q_OS_WIN
    const auto nativePath =
        modelPath.toStdWString();
    impl_->session =
        std::make_unique<Ort::Session>(
            impl_->environment,
            nativePath.c_str(),
            impl_->options);
#else
    const auto nativePath =
        modelPath.toUtf8();
    impl_->session =
        std::make_unique<Ort::Session>(
            impl_->environment,
            nativePath.constData(),
            impl_->options);
#endif

    auto &session = *impl_->session;
    Ort::AllocatorWithDefaultOptions allocator;

    if (session.GetInputCount() != 1
        || session.GetOutputCount() != 1) {
        throw std::runtime_error(
            "Gesture ONNX model must expose one input and one output");
    }

    if (std::string(
            session.GetInputNameAllocated(
                0,
                allocator)
                .get())
            != "features"
        || std::string(
               session.GetOutputNameAllocated(
                   0,
                   allocator)
                   .get())
            != "logits") {
        throw std::runtime_error(
            "Gesture ONNX IO names mismatch");
    }

    const auto inputInfo =
        session.GetInputTypeInfo(0)
            .GetTensorTypeAndShapeInfo();
    const auto inputShape =
        inputInfo.GetShape();

    if (inputInfo.GetElementType()
            != ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT
        || inputShape.size() != 3
        || (inputShape[0] != 1
            && inputShape[0] != -1)
        || inputShape[1] != windowSize_
        || inputShape[2]
            != static_cast<std::int64_t>(
                kGestureFeatureCount)) {
        throw std::runtime_error(
            "Expected gesture ONNX input float32 [1,T,134]");
    }

    const auto outputInfo =
        session.GetOutputTypeInfo(0)
            .GetTensorTypeAndShapeInfo();
    const auto outputShape =
        outputInfo.GetShape();

    if (outputInfo.GetElementType()
            != ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT
        || outputShape.size() != 2
        || outputShape[1]
            != static_cast<std::int64_t>(
                kLegacyOnnxGestureClassCount)) {
        throw std::runtime_error(
            "Expected legacy gesture ONNX output float32 [1,5]");
    }

    // Warm up before the model can be reported as active. M3 does not yet
    // map gesture predictions to system input, so this cannot trigger output.
    infer(
        std::vector<float>(
            static_cast<std::size_t>(windowSize_)
                * kGestureFeatureCount,
            0.0F));
#endif
}

OnnxGestureRecognizer::~OnnxGestureRecognizer() = default;

GestureScores OnnxGestureRecognizer::infer(
    const std::vector<float> &tensor)
{
#ifndef VC_WITH_ONNX
    Q_UNUSED(tensor);
    throw std::runtime_error(
        "ONNX gesture inference is disabled");
#else
    const std::size_t expected =
        static_cast<std::size_t>(windowSize_)
        * kGestureFeatureCount;

    if (tensor.size() != expected) {
        throw std::runtime_error(
            "Gesture ONNX tensor shape mismatch");
    }

    for (const float value : tensor) {
        if (!std::isfinite(value)) {
            throw std::runtime_error(
                "Gesture ONNX input contains non-finite values");
        }
    }

    std::array<std::int64_t, 3> dimensions{
        1,
        windowSize_,
        static_cast<std::int64_t>(
            kGestureFeatureCount)
    };

    auto memory =
        Ort::MemoryInfo::CreateCpu(
            OrtArenaAllocator,
            OrtMemTypeDefault);

    auto input =
        Ort::Value::CreateTensor<float>(
            memory,
            const_cast<float *>(tensor.data()),
            tensor.size(),
            dimensions.data(),
            dimensions.size());

    const char *inputName = "features";
    const char *outputName = "logits";

    auto values =
        impl_->session->Run(
            Ort::RunOptions{nullptr},
            &inputName,
            &input,
            1,
            &outputName,
            1);

    if (values.empty()
        || values[0]
               .GetTensorTypeAndShapeInfo()
               .GetElementCount()
            != kLegacyOnnxGestureClassCount) {
        throw std::runtime_error(
            "Gesture ONNX output size mismatch");
    }

    const auto *logits =
        values[0].GetTensorData<float>();

    const float maximum =
        *std::max_element(
            logits,
            logits + kLegacyOnnxGestureClassCount);

    GestureScores probabilities{};
    float sum = 0.0F;

    for (std::size_t index = 0;
         index < kLegacyOnnxGestureClassCount;
         ++index) {
        if (!std::isfinite(logits[index])) {
            throw std::runtime_error(
                "Gesture ONNX output contains non-finite logits");
        }

        const auto gesture = kLegacyOnnxGestureClasses[index];
        const auto gestureIndexValue = gestureIndex(gesture);
        probabilities[gestureIndexValue] =
            std::exp(logits[index] - maximum);
        sum += probabilities[gestureIndexValue];
    }

    if (!(sum > 0.0F)
        || !std::isfinite(sum)) {
        throw std::runtime_error(
            "Gesture ONNX softmax failed");
    }

    for (const auto gesture : kLegacyOnnxGestureClasses) {
        probabilities[gestureIndex(gesture)] /= sum;
    }

    return probabilities;
#endif
}

} // namespace vc
