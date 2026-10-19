#include "runtime/RuntimeMetrics.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace vc {

RuntimeMetrics::RuntimeMetrics(
    std::size_t windowSize)
    : windowSize_(
          std::max<std::size_t>(
              16U,
              windowSize))
{
}

std::uint64_t RuntimeMetrics::recordFrameReceived(
    std::int64_t captureUs)
{
    const auto sequence =
        receivedFrames_.fetch_add(
            1,
            std::memory_order_relaxed)
        + 1;

    const auto previousUs = lastFrameReceivedUs_.exchange(
        captureUs, std::memory_order_relaxed);
    if (previousUs > 0 && captureUs > previousUs) {
        std::lock_guard lock(samplesMutex_);
        pushBounded(cameraIntervalUs_, captureUs - previousUs, windowSize_);
    }

    return sequence;
}

void RuntimeMetrics::recordFrameReplaced()
{
    replacedFrames_.fetch_add(
        1,
        std::memory_order_relaxed);
}

void RuntimeMetrics::pushBounded(
    std::deque<std::int64_t> &samples,
    std::int64_t value,
    std::size_t limit)
{
    samples.push_back(
        std::max<std::int64_t>(0, value));

    while (samples.size() > limit) {
        samples.pop_front();
    }
}

void RuntimeMetrics::recordProcessed(
    std::int64_t conversionUs,
    std::int64_t inferenceUs,
    std::int64_t pipelineLatencyUs,
    std::int64_t gestureUs)
{
    processedFrames_.fetch_add(
        1,
        std::memory_order_relaxed);

    latestPipelineLatencyUs_.store(
        std::max<std::int64_t>(
            0,
            pipelineLatencyUs),
        std::memory_order_relaxed);

    std::lock_guard lock(samplesMutex_);

    pushBounded(
        conversionUs_,
        conversionUs,
        windowSize_);

    pushBounded(
        inferenceUs_,
        inferenceUs,
        windowSize_);

    pushBounded(
        pipelineLatencyUs_,
        pipelineLatencyUs,
        windowSize_);
    pushBounded(gestureUs_, gestureUs, windowSize_);
}

void RuntimeMetrics::recordOutputDispatch(std::int64_t dispatchUs)
{
    std::lock_guard lock(samplesMutex_);
    pushBounded(outputDispatchUs_, dispatchUs, windowSize_);
}

void RuntimeMetrics::recordGuiQueueDelay(std::int64_t delayUs)
{
    std::lock_guard lock(samplesMutex_);
    pushBounded(guiQueueDelayUs_, delayUs, windowSize_);
}

void RuntimeMetrics::recordGuiBackpressureWait(std::int64_t delayUs)
{
    std::lock_guard lock(samplesMutex_);
    pushBounded(guiBackpressureWaitUs_, delayUs, windowSize_);
}

void RuntimeMetrics::recordInputSubmitLatency(std::int64_t latencyUs)
{
    std::lock_guard lock(samplesMutex_);
    pushBounded(inputSubmitLatencyUs_, latencyUs, windowSize_);
}

int RuntimeMetrics::percentileMs(
    const std::deque<std::int64_t> &samples,
    double percentile)
{
    if (samples.empty()) {
        return 0;
    }

    std::vector<std::int64_t> sorted(
        samples.begin(),
        samples.end());

    std::sort(
        sorted.begin(),
        sorted.end());

    const double bounded =
        std::clamp(
            percentile,
            0.0,
            1.0);

    const auto index =
        static_cast<std::size_t>(
            std::ceil(
                bounded
                * static_cast<double>(
                    sorted.size() - 1U)));

    return static_cast<int>(
        (sorted[index] + 500) / 1000);
}

RuntimeMetricsSnapshot RuntimeMetrics::snapshot(
    std::int64_t nowUs)
{
    RuntimeMetricsSnapshot result;

    result.receivedFrames =
        receivedFrames_.load(
            std::memory_order_relaxed);

    result.processedFrames =
        processedFrames_.load(
            std::memory_order_relaxed);

    result.replacedFrames =
        replacedFrames_.load(
            std::memory_order_relaxed);

    result.lastFrameReceivedUs =
        lastFrameReceivedUs_.load(
            std::memory_order_relaxed);

    result.latestPipelineLatencyMs =
        static_cast<int>(
            (latestPipelineLatencyUs_.load(
                 std::memory_order_relaxed)
             + 500)
            / 1000);

    if (result.receivedFrames > 0U) {
        result.replacedPercent =
            static_cast<double>(
                result.replacedFrames)
            * 100.0
            / static_cast<double>(
                result.receivedFrames);
    }

    {
        std::lock_guard lock(samplesMutex_);

        result.pipelineLatencyP50Ms =
            percentileMs(
                pipelineLatencyUs_,
                0.50);

        result.pipelineLatencyP95Ms =
            percentileMs(
                pipelineLatencyUs_,
                0.95);

        result.inferenceP50Ms =
            percentileMs(
                inferenceUs_,
                0.50);

        result.inferenceP95Ms =
            percentileMs(
                inferenceUs_,
                0.95);

        result.conversionP50Ms =
            percentileMs(
                conversionUs_,
                0.50);

        result.conversionP95Ms =
            percentileMs(
                conversionUs_,
                0.95);
        result.cameraIntervalP50Ms = percentileMs(cameraIntervalUs_, 0.50);
        result.mediaPipeP50Ms = percentileMs(inferenceUs_, 0.50);
        result.gestureP50Ms = percentileMs(gestureUs_, 0.50);
        result.gestureP95Ms = percentileMs(gestureUs_, 0.95);
        result.outputDispatchP50Ms = percentileMs(outputDispatchUs_, 0.50);
        result.outputDispatchP95Ms = percentileMs(outputDispatchUs_, 0.95);
        result.guiQueueP95Ms = percentileMs(guiQueueDelayUs_, 0.95);
        result.guiBackpressureP95Ms = percentileMs(guiBackpressureWaitUs_, 0.95);
        result.inputSubmitP95Ms = percentileMs(inputSubmitLatencyUs_, 0.95);
    }

    {
        std::lock_guard lock(snapshotMutex_);

        if (lastSnapshotUs_ > 0
            && nowUs > lastSnapshotUs_) {

            const double elapsedSeconds =
                static_cast<double>(
                    nowUs - lastSnapshotUs_)
                / 1000000.0;

            const auto receivedDelta =
                result.receivedFrames
                - lastSnapshotReceived_;

            const auto processedDelta =
                result.processedFrames
                - lastSnapshotProcessed_;

            result.cameraFps =
                static_cast<int>(
                    std::lround(
                        static_cast<double>(
                            receivedDelta)
                        / elapsedSeconds));

            result.processedFps =
                static_cast<int>(
                    std::lround(
                        static_cast<double>(
                            processedDelta)
                        / elapsedSeconds));
        }

        lastSnapshotUs_ = nowUs;
        lastSnapshotReceived_ =
            result.receivedFrames;
        lastSnapshotProcessed_ =
            result.processedFrames;
    }

    return result;
}

} // namespace vc
