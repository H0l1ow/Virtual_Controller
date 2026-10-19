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
    metrics.recordProcessed(5'000, 13'000, 23'000, 7'000);
    metrics.recordOutputDispatch(2'000);
    metrics.recordOutputDispatch(3'000);
    metrics.recordGuiQueueDelay(12'000);
    metrics.recordGuiQueueDelay(48'000);
    metrics.recordGuiBackpressureWait(8'000);
    metrics.recordGuiBackpressureWait(45'000);
    metrics.recordInputSubmitLatency(25'000);
    metrics.recordInputSubmitLatency(75'000);

    const auto first = metrics.snapshot(2'000'000);

    assert(first.receivedFrames == 10);
    assert(first.processedFrames == 4);
    assert(first.replacedFrames == 3);
    assert(std::abs(first.replacedPercent - 30.0) < 0.001);
    assert(first.pipelineLatencyP50Ms == 23);
    assert(first.pipelineLatencyP95Ms == 35);
    assert(first.inferenceP50Ms == 13);
    assert(first.inferenceP95Ms == 20);
    assert(first.conversionP50Ms == 4);
    assert(first.conversionP95Ms == 5);
    assert(first.gestureP50Ms == 0); // existing call sites default to 0
    assert(first.gestureP95Ms == 7);
    assert(first.outputDispatchP50Ms == 3);
    assert(first.guiQueueP95Ms == 48);
    assert(first.guiBackpressureP95Ms == 45);
    assert(first.inputSubmitP95Ms == 75);
    assert(first.cameraIntervalP50Ms == 10);
    assert(first.mediaPipeP50Ms == first.inferenceP50Ms);

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
