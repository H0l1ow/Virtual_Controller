#pragma once

#include "control/ContinuousControlInterpreter.hpp"
#include "control/ControllerState.hpp"
#include "input/OutputService.hpp"
#include "runtime/LatestFrameSlot.hpp"
#include "runtime/RuntimeMetrics.hpp"
#include "tracking/TrackingTypes.hpp"

#include <QCamera>
#include <QCameraDevice>
#include <QCameraFormat>
#include <QMediaCaptureSession>
#include <QMediaDevices>
#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariant>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <stop_token>
#include <thread>

namespace vc {

class RuntimeController final : public QObject
{
    Q_OBJECT

public:
    enum class RuntimeState {
        Stopped,
        Starting,
        Running,
        Stopping,
        Faulted
    };
    Q_ENUM(RuntimeState)

    Q_PROPERTY(
        RuntimeState runtimeState
            READ runtimeState
                NOTIFY stateChanged)

    Q_PROPERTY(
        QString runtimeStateName
            READ runtimeStateName
                NOTIFY stateChanged)

    Q_PROPERTY(
        bool canStart
            READ canStart
                NOTIFY stateChanged)

    Q_PROPERTY(
        bool canStop
            READ canStop
                NOTIFY stateChanged)

    Q_PROPERTY(
        bool cameraRunning
            READ cameraRunning
                NOTIFY stateChanged)

    Q_PROPERTY(
        bool pipelineRunning
            READ pipelineRunning
                NOTIFY stateChanged)

    Q_PROPERTY(
        bool trackerReady
            READ trackerReady
                NOTIFY stateChanged)

    Q_PROPERTY(
        QStringList cameraNames
            READ cameraNames
                NOTIFY devicesChanged)

    Q_PROPERTY(
        QStringList formatNames
            READ formatNames
                NOTIFY devicesChanged)

    Q_PROPERTY(
        int cameraIndex
            READ cameraIndex
                NOTIFY devicesChanged)

    Q_PROPERTY(
        int formatIndex
            READ formatIndex
                NOTIFY devicesChanged)

    // Backward-compatible aliases used by the existing M1 QML.
    Q_PROPERTY(
        int fps
            READ processedFps
                NOTIFY stateChanged)

    Q_PROPERTY(
        int latencyMs
            READ latestLatencyMs
                NOTIFY stateChanged)

    Q_PROPERTY(
        int cameraFps
            READ cameraFps
                NOTIFY stateChanged)

    Q_PROPERTY(
        int processedFps
            READ processedFps
                NOTIFY stateChanged)

    Q_PROPERTY(
        qulonglong replacedFrames
            READ replacedFrames
                NOTIFY stateChanged)

    Q_PROPERTY(
        double replacedPercent
            READ replacedPercent
                NOTIFY stateChanged)

    Q_PROPERTY(
        int latencyP50Ms
            READ latencyP50Ms
                NOTIFY stateChanged)

    Q_PROPERTY(
        int latencyP95Ms
            READ latencyP95Ms
                NOTIFY stateChanged)

    Q_PROPERTY(
        int inferenceP50Ms
            READ inferenceP50Ms
                NOTIFY stateChanged)

    Q_PROPERTY(
        int inferenceP95Ms
            READ inferenceP95Ms
                NOTIFY stateChanged)

    Q_PROPERTY(
        int conversionP50Ms
            READ conversionP50Ms
                NOTIFY stateChanged)

    Q_PROPERTY(
        int conversionP95Ms
            READ conversionP95Ms
                NOTIFY stateChanged)

    Q_PROPERTY(
        QString resolution
            READ resolution
                NOTIFY stateChanged)

    Q_PROPERTY(
        QString trackingStatus
            READ trackingStatus
                NOTIFY stateChanged)

    Q_PROPERTY(
        QString errorMessage
            READ errorMessage
                NOTIFY stateChanged)

    Q_PROPERTY(
        bool leftTracked
            READ leftTracked
                NOTIFY stateChanged)

    Q_PROPERTY(
        bool rightTracked
            READ rightTracked
                NOTIFY stateChanged)

    Q_PROPERTY(
        double leftConfidence
            READ leftConfidence
                NOTIFY stateChanged)

    Q_PROPERTY(
        double rightConfidence
            READ rightConfidence
                NOTIFY stateChanged)

    Q_PROPERTY(
        QString leftReportedSide
            READ leftReportedSide
                NOTIFY stateChanged)

    Q_PROPERTY(
        QString rightReportedSide
            READ rightReportedSide
                NOTIFY stateChanged)

    Q_PROPERTY(
        QVariantList leftLandmarks
            READ leftLandmarks
                NOTIFY stateChanged)

    Q_PROPERTY(
        QVariantList rightLandmarks
            READ rightLandmarks
                NOTIFY stateChanged)

    Q_PROPERTY(
        int detectionConfidence
            READ detectionConfidence
                WRITE setDetectionConfidence
                    NOTIFY settingsChanged)

    Q_PROPERTY(
        int trackingConfidence
            READ trackingConfidence
                WRITE setTrackingConfidence
                    NOTIFY settingsChanged)

    Q_PROPERTY(
        bool swapHandedness
            READ swapHandedness
                WRITE setSwapHandedness
                    NOTIFY settingsChanged)

    Q_PROPERTY(
        bool cursorControlAvailable
            READ cursorControlAvailable
                CONSTANT)

    Q_PROPERTY(
        bool outputAvailable
            READ outputAvailable
                NOTIFY stateChanged)

    Q_PROPERTY(
        bool outputArmed
            READ outputArmed
                WRITE setOutputArmed
                    NOTIFY stateChanged)

    Q_PROPERTY(
        QString outputError
            READ outputError
                NOTIFY stateChanged)

    Q_PROPERTY(
        int cursorX
            READ cursorX
                NOTIFY stateChanged)

    Q_PROPERTY(
        int cursorY
            READ cursorY
                NOTIFY stateChanged)

    Q_PROPERTY(
        int cursorScreenWidth
            READ cursorScreenWidth
                NOTIFY stateChanged)

    Q_PROPERTY(
        int cursorScreenHeight
            READ cursorScreenHeight
                NOTIFY stateChanged)

    Q_PROPERTY(
        int sensitivity
            READ sensitivity
                WRITE setSensitivity
                    NOTIFY settingsChanged)

    Q_PROPERTY(
        bool smoothing
            READ smoothing
                WRITE setSmoothing
                    NOTIFY settingsChanged)

    Q_PROPERTY(
        int cursorSpeedPercent
            READ cursorSpeedPercent
                WRITE setCursorSpeedPercent
                    NOTIFY settingsChanged)

    Q_PROPERTY(
        bool invertX
            READ invertX
                WRITE setInvertX
                    NOTIFY settingsChanged)

    Q_PROPERTY(
        bool invertY
            READ invertY
                WRITE setInvertY
                    NOTIFY settingsChanged)

    Q_PROPERTY(
        int deadzone
            READ deadzone
                WRITE setDeadzone
                    NOTIFY settingsChanged)

public:
    explicit RuntimeController(
        QObject *parent = nullptr);

    ~RuntimeController() override;

    bool shutdownForExit(
        std::chrono::milliseconds timeout);

    RuntimeState runtimeState() const
    {
        return runtimeState_;
    }

    QString runtimeStateName() const;

    bool canStart() const
    {
        return runtimeState_ == RuntimeState::Stopped
            || runtimeState_ == RuntimeState::Faulted;
    }

    bool canStop() const
    {
        return runtimeState_ == RuntimeState::Starting
            || runtimeState_ == RuntimeState::Running;
    }

    bool cameraRunning() const
    {
        return cameraRunning_;
    }

    // A session remains logically active while its native tracker is being
    // destroyed. This prevents Start from racing a previous Stop teardown.
    bool pipelineRunning() const
    {
        return runtimeState_ == RuntimeState::Starting
            || runtimeState_ == RuntimeState::Running
            || runtimeState_ == RuntimeState::Stopping;
    }

    bool trackerReady() const
    {
        return trackerReady_;
    }

    QStringList cameraNames() const;
    QStringList formatNames() const;

    int cameraIndex() const
    {
        return cameraIndex_;
    }

    int formatIndex() const
    {
        return formatIndex_;
    }

    int cameraFps() const
    {
        return metricsSnapshot_.cameraFps;
    }

    int processedFps() const
    {
        return metricsSnapshot_.processedFps;
    }

    qulonglong replacedFrames() const
    {
        return static_cast<qulonglong>(
            metricsSnapshot_.replacedFrames);
    }

    double replacedPercent() const
    {
        return metricsSnapshot_.replacedPercent;
    }

    int latestLatencyMs() const
    {
        return metricsSnapshot_.latestPipelineLatencyMs;
    }

    int latencyP50Ms() const
    {
        return metricsSnapshot_.pipelineLatencyP50Ms;
    }

    int latencyP95Ms() const
    {
        return metricsSnapshot_.pipelineLatencyP95Ms;
    }

    int inferenceP50Ms() const
    {
        return metricsSnapshot_.inferenceP50Ms;
    }

    int inferenceP95Ms() const
    {
        return metricsSnapshot_.inferenceP95Ms;
    }

    int conversionP50Ms() const
    {
        return metricsSnapshot_.conversionP50Ms;
    }

    int conversionP95Ms() const
    {
        return metricsSnapshot_.conversionP95Ms;
    }

    QString resolution() const
    {
        return resolution_;
    }

    QString trackingStatus() const;

    QString errorMessage() const
    {
        return errorMessage_;
    }

    bool leftTracked() const
    {
        return leftTracked_;
    }

    bool rightTracked() const
    {
        return rightTracked_;
    }

    double leftConfidence() const
    {
        return leftHandednessConfidence_;
    }

    double rightConfidence() const
    {
        return rightHandednessConfidence_;
    }

    QString leftReportedSide() const
    {
        return leftReportedSide_;
    }

    QString rightReportedSide() const
    {
        return rightReportedSide_;
    }

    QVariantList leftLandmarks() const
    {
        return leftLandmarks_;
    }

    QVariantList rightLandmarks() const
    {
        return rightLandmarks_;
    }

    int detectionConfidence() const
    {
        return detectionConfidence_;
    }

    int trackingConfidence() const
    {
        return trackingConfidence_;
    }

    bool swapHandedness() const
    {
        return swapHandedness_.load();
    }

    bool cursorControlAvailable() const
    {
        return true;
    }

    bool outputAvailable() const
    {
        return outputSnapshot_.ready;
    }

    bool outputArmed() const
    {
        return outputSnapshot_.armed;
    }

    QString outputError() const
    {
        return QString::fromStdString(
            outputSnapshot_.error);
    }

    int cursorX() const
    {
        return cursorX_;
    }

    int cursorY() const
    {
        return cursorY_;
    }

    int cursorScreenWidth() const
    {
        return cursorScreenWidth_;
    }

    int cursorScreenHeight() const
    {
        return cursorScreenHeight_;
    }

    int sensitivity() const
    {
        return sensitivity_;
    }

    bool smoothing() const
    {
        return smoothing_;
    }

    int cursorSpeedPercent() const
    {
        return cursorSpeedPercent_;
    }

    bool invertX() const
    {
        return invertX_;
    }

    bool invertY() const
    {
        return invertY_;
    }

    int deadzone() const
    {
        return deadzone_;
    }

    void setDetectionConfidence(int value);
    void setTrackingConfidence(int value);
    void setSwapHandedness(bool value);
    void setOutputArmed(bool value);
    void setSensitivity(int value);
    void setSmoothing(bool value);
    void setCursorSpeedPercent(int value);
    void setInvertX(bool value);
    void setInvertY(bool value);
    void setDeadzone(int value);

    Q_INVOKABLE void attachVideoOutput(QObject *output);
    Q_INVOKABLE void selectCamera(int index);
    Q_INVOKABLE void selectFormat(int index);

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void togglePipeline();
    Q_INVOKABLE void requestApplicationExit();

signals:
    void stateChanged();
    void devicesChanged();
    void settingsChanged();

    // First signal lets main.cpp arm its process-level watchdog. The second is
    // emitted only after the current tracking session reached a safe terminal
    // state, so the Qt event loop can exit gracefully.
    void applicationExitRequested();
    void applicationExitReady();

private:
    struct WorkerSession
    {
        std::shared_ptr<LatestFrameSlot> inbox;
        std::shared_ptr<RuntimeMetrics> metrics;

        QString libraryPath;
        QString modelPath;

        float detectionConfidence{};
        float trackingConfidence{};

        std::uint64_t generation{};
    };

    struct WorkerExitState
    {
        std::mutex mutex;
        std::condition_variable condition;
        bool exited{};
    };

    void setRuntimeState(RuntimeState state);
    void failWithoutSession(const QString &message);
    void requestSessionStop(RuntimeState terminalState);
    void finalizeSessionStop(std::uint64_t generation);
    void handleWorkerFault(
        std::uint64_t generation,
        const QString &message);
    void handleWorkerSessionFinished(
        std::uint64_t generation);
    void maybeEmitApplicationExitReady();

    void teardownCameraAndFramePath(
        const std::shared_ptr<LatestFrameSlot> &stoppingInbox);

    void refreshDevices();
    void refreshFormats();

    QString mediaPipeLibraryPath() const;
    QString handLandmarkerModelPath() const;

    void clearTracking();
    void resetContinuousControl();
    void updateContinuousControl(
        const TrackingFrame &frame);
    void refreshOutputStatus();
    void updateCursorGeometry();

    void applyTrackingFrame(
        const TrackingFrame &frame,
        std::uint64_t generation);

    void workerLoop(
        std::stop_token stopToken);

    void updateStats();

    static QVariantList toVariantLandmarks(
        const HandTrackingState &hand);

    QMediaDevices mediaDevices_;
    QList<QCameraDevice> cameras_;
    QList<QCameraFormat> formats_;

    std::unique_ptr<QCamera> camera_;
    QMediaCaptureSession captureSession_;
    QPointer<QObject> videoOutput_;

    QMetaObject::Connection frameConnection_;
    QMetaObject::Connection cameraActiveConnection_;
    QMetaObject::Connection cameraErrorConnection_;

    std::shared_ptr<LatestFrameSlot> inbox_;
    std::shared_ptr<RuntimeMetrics> metrics_;

    std::mutex workerMutex_;
    std::condition_variable workerCondition_;
    std::optional<WorkerSession> pendingWorkerSession_;
    std::optional<std::uint64_t> activeWorkerGeneration_;

    std::shared_ptr<WorkerExitState> workerExitState_;
    std::jthread worker_;

    QTimer statsTimer_;
    RuntimeMetricsSnapshot metricsSnapshot_;

    std::atomic_bool swapHandedness_{};

    ContinuousControlInterpreter continuousControl_;
    OutputService outputService_;
    OutputStatus outputSnapshot_{};
    ControllerState controllerState_{};

    int sensitivity_{70};
    int cursorSpeedPercent_{250};
    int deadzone_{12};
    bool smoothing_{true};
    bool invertX_{false};
    bool invertY_{false};

    int cursorX_{};
    int cursorY_{};
    double cursorAccumulatorX_{};
    double cursorAccumulatorY_{};
    int cursorScreenWidth_{1920};
    int cursorScreenHeight_{1080};
    bool cursorPreviewInitialized_{};

    std::uint64_t generation_{};

    RuntimeState runtimeState_{RuntimeState::Stopped};
    RuntimeState terminalStateAfterStop_{RuntimeState::Stopped};
    bool exitRequested_{};

    int cameraIndex_{};
    int formatIndex_{};

    int detectionConfidence_{60};
    int trackingConfidence_{55};

    bool cameraRunning_{};
    bool trackerReady_{};

    QString resolution_{"-"};
    QString errorMessage_;

    bool leftTracked_{};
    bool rightTracked_{};

    double leftHandednessConfidence_{};
    double rightHandednessConfidence_{};
    QString leftReportedSide_{"-"};
    QString rightReportedSide_{"-"};

    QVariantList leftLandmarks_;
    QVariantList rightLandmarks_;
};

} // namespace vc
