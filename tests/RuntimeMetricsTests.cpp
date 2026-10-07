#include "runtime/RuntimeMetrics.hpp"

#include <cassert>
#include <cmath>

int main()
{
    vc::RuntimeMetrics metrics(32);

    for (int i = 0; i < 10; ++i) {
        metrics.recordFrameReceived(
            1'000'000 + i * 10'000);
    }

    for (int i = 0; i < 3; ++i) {
        metrics.recordFrameReplaced();
    }

    metrics.recordProcessed(2'000, 10'000, 18'000);
    metrics.recordProcessed(3'000, 12'000, 22'000);
    metrics.recordProcessed(4'000, 20'000, 35'000);

    const auto first = metrics.snapshot(2'000'000);

    assert(first.receivedFrames == 10);
    assert(first.processedFrames == 3);
    assert(first.replacedFrames == 3);
    assert(std::abs(first.replacedPercent - 30.0) < 0.001);
    assert(first.pipelineLatencyP50Ms == 22);
    assert(first.pipelineLatencyP95Ms == 35);
    assert(first.inferenceP50Ms == 12);
    assert(first.inferenceP95Ms == 20);
    assert(first.conversionP50Ms == 3);
    assert(first.conversionP95Ms == 4);

    for (int i = 0; i < 20; ++i) {
        metrics.recordFrameReceived(
            2'100'000 + i * 10'000);
    }

    for (int i = 0; i < 10; ++i) {
        metrics.recordProcessed(2'000, 9'000, 15'000);
    }

    const auto second = metrics.snapshot(3'000'000);

    assert(second.cameraFps == 20);
    assert(second.processedFps == 10);
    assert(second.latestPipelineLatencyMs == 15);

    return 0;
}
