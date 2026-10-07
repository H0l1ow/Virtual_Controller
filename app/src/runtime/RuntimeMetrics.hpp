#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>

namespace vc {

struct RuntimeMetricsSnapshot {
    std::uint64_t receivedFrames{};
    std::uint64_t processedFrames{};
    std::uint64_t replacedFrames{};

    int cameraFps{};
    int processedFps{};
    double replacedPercent{};

    int latestPipelineLatencyMs{};
    int pipelineLatencyP50Ms{};
    int pipelineLatencyP95Ms{};
    int inferenceP50Ms{};
    int inferenceP95Ms{};
    int conversionP50Ms{};
    int conversionP95Ms{};

    std::int64_t lastFrameReceivedUs{};
};

class RuntimeMetrics final
{
public:
    explicit RuntimeMetrics(
        std::size_t windowSize = 180U);

    std::uint64_t recordFrameReceived(
        std::int64_t captureUs);

    void recordFrameReplaced();

    void recordProcessed(
        std::int64_t conversionUs,
        std::int64_t inferenceUs,
        std::int64_t pipelineLatencyUs);

    RuntimeMetricsSnapshot snapshot(
        std::int64_t nowUs);

private:
    static int percentileMs(
        const std::deque<std::int64_t> &samples,
        double percentile);

    static void pushBounded(
        std::deque<std::int64_t> &samples,
        std::int64_t value,
        std::size_t limit);

    const std::size_t windowSize_;

    std::atomic<std::uint64_t> receivedFrames_{};
    std::atomic<std::uint64_t> processedFrames_{};
    std::atomic<std::uint64_t> replacedFrames_{};
    std::atomic<std::int64_t> lastFrameReceivedUs_{};
    std::atomic<std::int64_t> latestPipelineLatencyUs_{};

    std::mutex samplesMutex_;
    std::deque<std::int64_t> pipelineLatencyUs_;
    std::deque<std::int64_t> inferenceUs_;
    std::deque<std::int64_t> conversionUs_;

    std::mutex snapshotMutex_;
    std::int64_t lastSnapshotUs_{};
    std::uint64_t lastSnapshotReceived_{};
    std::uint64_t lastSnapshotProcessed_{};
};

} // namespace vc
