#pragma once

#include "control/ContinuousControlInterpreter.hpp"
#include "control/ControllerState.hpp"
#include "gestures/GestureTypes.hpp"
#include "gestures/GestureDataRecorder.hpp"
#include "gestures/GestureReplayReader.hpp"
#include "gestures/GestureEngine.hpp"
#include "gestures/GestureCalibrator.hpp"
#include "gestures/HandRangeCalibrator.hpp"
#include "gestures/GestureTuning.hpp"
#include "control/CursorPlayground.hpp"
#include "gestures/GestureStateManager.hpp"
#include "mapping/ActionMapper.hpp"
#include "mapping/ProfileStore.hpp"
#include "mapping/MappingValidator.hpp"
#include "input/OutputService.hpp"
#include "runtime/LatestFrameSlot.hpp"
#include "runtime/LatestValueMailbox.hpp"
#include "runtime/GuiResultBackpressure.hpp"
#include "runtime/PreviewFrameLimiter.hpp"
#include "runtime/PreviewRoutingPolicy.hpp"
#include "runtime/RuntimeMetrics.hpp"
#include "runtime/CameraReconnectPolicy.hpp"
#include "tracking/TrackingTypes.hpp"

#include <QCamera>
#include <QCameraDevice>
#include <QCameraFormat>
#include <QByteArray>
#include <QMediaCaptureSession>
#include <QMediaDevices>
#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QVariant>
#include <QVariantMap>
#include <QVideoSink>
#include <QVideoFrame>

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

    Q_PROPERTY(int guiQueueP95Ms READ guiQueueP95Ms NOTIFY stateChanged)
    Q_PROPERTY(int guiBackpressureP95Ms READ guiBackpressureP95Ms NOTIFY stateChanged)
    Q_PROPERTY(int inputSubmitP95Ms READ inputSubmitP95Ms NOTIFY stateChanged)

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

    Q_PROPERTY(bool autoRecoverCamera READ autoRecoverCamera WRITE setAutoRecoverCamera NOTIFY settingsChanged)
    Q_PROPERTY(QString cameraRecoveryStatus READ cameraRecoveryStatus NOTIFY stateChanged)

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

    Q_PROPERTY(bool outputActive READ outputActive NOTIFY stateChanged)
    Q_PROPERTY(QString outputSuspendReason READ outputSuspendReason NOTIFY stateChanged)

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
        bool cursorFrozen
            READ cursorFrozen
                NOTIFY stateChanged)

    Q_PROPERTY(
        bool cursorMovementLocked
            READ cursorMovementLocked
                NOTIFY stateChanged)

    // M4.5 Gesture Playground observes the backend-neutral logical state even
    // while System output is disarmed. This lets the in-app exercises test the
    // complete GestureStateManager -> ActionMapper path without sending input
    // to Windows.
    Q_PROPERTY(
        bool logicalMouseLeft
            READ logicalMouseLeft
                NOTIFY stateChanged)

    Q_PROPERTY(
        bool logicalMouseRight
            READ logicalMouseRight
                NOTIFY stateChanged)

    Q_PROPERTY(
        double logicalWheel
            READ logicalWheel
                NOTIFY stateChanged)

    Q_PROPERTY(
        QStringList logicalKeys
            READ logicalKeys
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

    Q_PROPERTY(
        bool gestureRecognitionAvailable
            READ gestureRecognitionAvailable
                CONSTANT)

    Q_PROPERTY(
        QString leftGesture
            READ leftGesture
                NOTIFY stateChanged)

    Q_PROPERTY(
        QString rightGesture
            READ rightGesture
                NOTIFY stateChanged)

    Q_PROPERTY(
        double leftGestureConfidence
            READ leftGestureConfidence
                NOTIFY stateChanged)

    Q_PROPERTY(
        double rightGestureConfidence
            READ rightGestureConfidence
                NOTIFY stateChanged)

    Q_PROPERTY(
        int recognitionThreshold
            READ recognitionThreshold
                WRITE setRecognitionThreshold
                    NOTIFY settingsChanged)

    Q_PROPERTY(
        int debounceMs
            READ debounceMs
                WRITE setDebounceMs
                    NOTIFY settingsChanged)

    Q_PROPERTY(
        int cooldownMs
            READ cooldownMs
                WRITE setCooldownMs
                    NOTIFY settingsChanged)

    Q_PROPERTY(
        bool requireRelease
            READ requireRelease
                WRITE setRequireRelease
                    NOTIFY settingsChanged)

    Q_PROPERTY(
        QString leftGestureEvent
            READ leftGestureEvent
                NOTIFY stateChanged)

    Q_PROPERTY(
        QString rightGestureEvent
            READ rightGestureEvent
                NOTIFY stateChanged)

    Q_PROPERTY(
        QStringList mappingProfileNames
            READ mappingProfileNames
                NOTIFY mappingChanged)

    Q_PROPERTY(
        QString activeProfile
            READ activeProfile
                WRITE setActiveProfile
                    NOTIFY mappingChanged)

    Q_PROPERTY(
        QVariantList mappingRows
            READ mappingRows
                NOTIFY mappingChanged)

    Q_PROPERTY(QVariantList mappingIssues READ mappingIssues NOTIFY mappingChanged)
    Q_PROPERTY(bool canDeleteActiveProfile READ canDeleteActiveProfile NOTIFY mappingChanged)

    Q_PROPERTY(
        QString mappingError
            READ mappingError
                NOTIFY mappingChanged)

    Q_PROPERTY(
        QString mappingProfileDirectory
            READ mappingProfileDirectory
                NOTIFY mappingChanged)

    Q_PROPERTY(
        QString gestureBackendName
            READ gestureBackendName
                NOTIFY stateChanged)

    Q_PROPERTY(
        QString gestureModelStatus
            READ gestureModelStatus
                NOTIFY stateChanged)

    Q_PROPERTY(
        bool gestureModelActive
            READ gestureModelActive
                NOTIFY stateChanged)

    Q_PROPERTY(QVariantList gestureDiagnostics READ gestureDiagnostics NOTIFY stateChanged)
    Q_PROPERTY(bool rangeCalibrationActive READ rangeCalibrationActive NOTIFY stateChanged)
    Q_PROPERTY(int rangeCalibrationProgress READ rangeCalibrationProgress NOTIFY stateChanged)
    Q_PROPERTY(QString rangeCalibrationStatus READ rangeCalibrationStatus NOTIFY stateChanged)
    Q_PROPERTY(QVariantList rangeCalibrationResults READ rangeCalibrationResults NOTIFY stateChanged)
    Q_PROPERTY(QVariantMap handCalibration READ handCalibration NOTIFY settingsChanged)
    Q_PROPERTY(bool replayActive READ replayActive NOTIFY stateChanged)
    Q_PROPERTY(int replayFrames READ replayFrames NOTIFY stateChanged)
    Q_PROPERTY(QString replayStatus READ replayStatus NOTIFY stateChanged)
    Q_PROPERTY(QString replayError READ replayError NOTIFY stateChanged)
    Q_PROPERTY(QString replayLeftGesture READ replayLeftGesture NOTIFY stateChanged)
    Q_PROPERTY(QString replayRightGesture READ replayRightGesture NOTIFY stateChanged)
    Q_PROPERTY(QString replayLeftLabel READ replayLeftLabel NOTIFY stateChanged)
    Q_PROPERTY(QString replayRightLabel READ replayRightLabel NOTIFY stateChanged)
    Q_PROPERTY(int cameraIntervalP50Ms READ cameraIntervalP50Ms NOTIFY stateChanged)
    Q_PROPERTY(int mediaPipeP50Ms READ mediaPipeP50Ms NOTIFY stateChanged)
    Q_PROPERTY(int gestureP50Ms READ gestureP50Ms NOTIFY stateChanged)
    Q_PROPERTY(int gestureP95Ms READ gestureP95Ms NOTIFY stateChanged)
    Q_PROPERTY(int outputDispatchP50Ms READ outputDispatchP50Ms NOTIFY stateChanged)
    Q_PROPERTY(int outputDispatchP95Ms READ outputDispatchP95Ms NOTIFY stateChanged)
    Q_PROPERTY(int gestureGraceMs READ gestureGraceMs WRITE setGestureGraceMs NOTIFY settingsChanged)
    Q_PROPERTY(int gestureReleaseThreshold READ gestureReleaseThreshold WRITE setGestureReleaseThreshold NOTIFY settingsChanged)
    // M5.3 thresholds inherit the existing global Recognition threshold (-1).
    Q_PROPERTY(QVariantMap gestureThresholds READ gestureThresholds NOTIFY settingsChanged)
    Q_PROPERTY(QVariantMap gestureThresholdOverrides READ gestureThresholdOverrides NOTIFY settingsChanged)
    Q_PROPERTY(bool calibrationActive READ calibrationActive NOTIFY stateChanged)
    Q_PROPERTY(QString calibrationStatus READ calibrationStatus NOTIFY stateChanged)
    Q_PROPERTY(int calibrationProgress READ calibrationProgress NOTIFY stateChanged)
    Q_PROPERTY(QString calibrationGesture READ calibrationGesture NOTIFY stateChanged)
    Q_PROPERTY(QString calibrationHand READ calibrationHand NOTIFY stateChanged)
    Q_PROPERTY(QVariantList calibrationResults READ calibrationResults NOTIFY stateChanged)
    Q_PROPERTY(bool cursorPlaygroundActive READ cursorPlaygroundActive NOTIFY stateChanged)
    Q_PROPERTY(QVariantMap cursorPlaygroundState READ cursorPlaygroundState NOTIFY stateChanged)
    Q_PROPERTY(bool recordingActive READ recordingActive NOTIFY stateChanged)
    Q_PROPERTY(int recordedFrames READ recordedFrames NOTIFY stateChanged)
    Q_PROPERTY(QString recordingPath READ recordingPath NOTIFY stateChanged)
    Q_PROPERTY(QString recordingError READ recordingError NOTIFY stateChanged)
    Q_PROPERTY(QVariantMap recordingQuality READ recordingQuality NOTIFY recordingQualityChanged)
    Q_PROPERTY(QString recordingsDirectory READ recordingsDirectory NOTIFY recordingsChanged)
    Q_PROPERTY(QVariantList recordings READ recordings NOTIFY recordingsChanged)
    Q_PROPERTY(QString recordingsError READ recordingsError NOTIFY recordingsChanged)

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
    int cameraIntervalP50Ms() const { return metricsSnapshot_.cameraIntervalP50Ms; }
    int mediaPipeP50Ms() const { return metricsSnapshot_.mediaPipeP50Ms; }
    int gestureP50Ms() const { return metricsSnapshot_.gestureP50Ms; }
    int gestureP95Ms() const { return metricsSnapshot_.gestureP95Ms; }
    int outputDispatchP50Ms() const { return metricsSnapshot_.outputDispatchP50Ms; }
    int outputDispatchP95Ms() const { return metricsSnapshot_.outputDispatchP95Ms; }
    int guiQueueP95Ms() const { return metricsSnapshot_.guiQueueP95Ms; }
    int guiBackpressureP95Ms() const { return metricsSnapshot_.guiBackpressureP95Ms; }
    int inputSubmitP95Ms() const { return metricsSnapshot_.inputSubmitP95Ms; }

    QString resolution() const
    {
        return resolution_;
    }

    QString trackingStatus() const;

    QString errorMessage() const
    {
        return errorMessage_;
    }

    bool autoRecoverCamera() const { return autoRecoverCamera_; }
    QString cameraRecoveryStatus() const { return cameraRecoveryStatus_; }
    void setAutoRecoverCamera(bool enabled);

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
        return outputSnapshot_.requested;
    }

    bool outputActive() const
    {
        return outputSnapshot_.armed;
    }

    QString outputSuspendReason() const
    {
        return QString::fromStdString(outputSnapshot_.reason);
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

    bool cursorFrozen() const
    {
        return controllerState_.cursorFrozen;
    }

    bool cursorMovementLocked() const
    {
        return controllerState_.cursorMovementLocked;
    }

    bool logicalMouseLeft() const
    {
        return controllerState_.mouseLeft;
    }

    bool logicalMouseRight() const
    {
        return controllerState_.mouseRight;
    }

    double logicalWheel() const
    {
        return static_cast<double>(controllerState_.wheel);
    }

    QStringList logicalKeys() const;

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

    bool gestureRecognitionAvailable() const
    {
        return true;
    }

    QString leftGesture() const
    {
        return leftGesture_;
    }

    QString rightGesture() const
    {
        return rightGesture_;
    }

    double leftGestureConfidence() const
    {
        return leftGestureConfidence_;
    }

    double rightGestureConfidence() const
    {
        return rightGestureConfidence_;
    }

    int recognitionThreshold() const
    {
        return recognitionThreshold_.load();
    }

    int debounceMs() const
    {
        return debounceMs_;
    }

    int cooldownMs() const
    {
        return cooldownMs_;
    }

    bool requireRelease() const
    {
        return requireRelease_;
    }

    QString leftGestureEvent() const
    {
        return leftGestureEvent_;
    }

    QString rightGestureEvent() const
    {
        return rightGestureEvent_;
    }

    QStringList mappingProfileNames() const
    {
        return mappingProfileNames_;
    }

    QString activeProfile() const
    {
        return activeProfile_;
    }

    QVariantList mappingRows() const;

    QString mappingError() const
    {
        return mappingError_;
    }

    QString mappingProfileDirectory() const;
    QVariantList mappingIssues() const;
    bool canDeleteActiveProfile() const;

    QString gestureBackendName() const
    {
        return gestureBackendName_;
    }

    QString gestureModelStatus() const
    {
        return gestureModelStatus_;
    }

    bool gestureModelActive() const
    {
        return gestureModelActive_;
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
    void setRecognitionThreshold(int value);
    void setDebounceMs(int value);
    void setCooldownMs(int value);
    void setRequireRelease(bool value);
    void setGestureGraceMs(int value);
    void setGestureReleaseThreshold(int value);
    QVariantMap gestureThresholds() const;
    QVariantMap gestureThresholdOverrides() const;
    Q_INVOKABLE void setGestureThreshold(const QString &gesture, int value);
    Q_INVOKABLE void resetGestureThresholds();
    bool calibrationActive() const { return calibrator_.capturing(); }
    QString calibrationStatus() const;
    int calibrationProgress() const { return calibrator_.progressPercent(); }
    QString calibrationGesture() const;
    QString calibrationHand() const;
    QVariantList calibrationResults() const;
    Q_INVOKABLE bool startGestureCalibration(const QString &hand, const QString &gesture);
    Q_INVOKABLE void cancelGestureCalibration();
    Q_INVOKABLE bool applyGestureCalibration(const QString &gesture);
    Q_INVOKABLE void clearGestureCalibration();
    bool cursorPlaygroundActive() const { return playgroundMode_; }
    QVariantMap cursorPlaygroundState() const;
    Q_INVOKABLE bool startCursorPlayground();
    Q_INVOKABLE void stopCursorPlayground();
    int gestureGraceMs() const { return gestureGraceMs_; }
    int gestureReleaseThreshold() const { return gestureReleaseThreshold_; }
    QVariantList gestureDiagnostics() const { return gestureDiagnostics_; }
    bool rangeCalibrationActive() const { return rangeCalibrator_.capturing(); }
    int rangeCalibrationProgress() const { return rangeCalibrator_.progress(); }
    QString rangeCalibrationStatus() const;
    QVariantList rangeCalibrationResults() const;
    QVariantMap handCalibration() const;
    Q_INVOKABLE bool startRangeCalibration(const QString &side, const QString &kind);
    Q_INVOKABLE void cancelRangeCalibration();
    Q_INVOKABLE bool applyRangeCalibration(const QString &side, const QString &kind);
    Q_INVOKABLE void resetRangeCalibration();
    Q_INVOKABLE void setHandCalibration(const QVariantMap &values);
    bool replayActive() const { return replayTimer_.isActive(); }
    int replayFrames() const { return replayFrames_; }
    QString replayStatus() const { return replayStatus_; }
    QString replayError() const { return replayError_; }
    QString replayLeftGesture() const { return replayGestures_[0]; }
    QString replayRightGesture() const { return replayGestures_[1]; }
    QString replayLeftLabel() const { return replayLabels_[0]; }
    QString replayRightLabel() const { return replayLabels_[1]; }
    Q_INVOKABLE bool startGestureReplay(const QString &filePath);
    Q_INVOKABLE void stopGestureReplay();
    Q_INVOKABLE bool reloadGestureModel();
    bool recordingActive() const { return recorder_.active(); }
    int recordedFrames() const { return static_cast<int>(recorder_.frameCount()); }
    QString recordingPath() const { return recordingPath_; }
    QString recordingError() const { return recordingError_; }
    QVariantMap recordingQuality() const;
    QString recordingsDirectory() const;
    QVariantList recordings() const;
    QString recordingsError() const { return recordingsError_; }
    Q_INVOKABLE bool setRecordingsDirectory(const QUrl &folder);
    Q_INVOKABLE void resetRecordingsDirectory();
    Q_INVOKABLE void openRecordingsFolder();
    Q_INVOKABLE bool openRecordingFile(const QString &name);
    Q_INVOKABLE bool deleteRecordingFile(const QString &name);
    Q_INVOKABLE bool replayRecordingFile(const QString &name);
    Q_INVOKABLE void refreshRecordings() { emit recordingsChanged(); }
    Q_INVOKABLE bool startGestureRecording(const QString &left, const QString &right);
    Q_INVOKABLE void stopGestureRecording();
    void setActiveProfile(const QString &name);

    Q_INVOKABLE void attachVideoOutput(QObject *output);
    Q_INVOKABLE void attachPiPVideoSink(QObject *sink);
    Q_INVOKABLE void attachRecordingVideoSink(QObject *sink);
    Q_INVOKABLE void setPiPActive(bool active);
    // Minimize/Show Desktop only changes the presentation route.
    // It never changes the persistent output request or stops MediaPipe.
    Q_INVOKABLE void setMainWindowMinimized(bool minimized);
    Q_INVOKABLE void selectCamera(int index);
    Q_INVOKABLE void selectFormat(int index);

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void togglePipeline();
    Q_INVOKABLE void requestApplicationExit();

    Q_INVOKABLE int addMapping();
    Q_INVOKABLE void removeMapping(int index);
    Q_INVOKABLE bool updateMapping(
        int index,
        const QString &hand,
        const QString &gesture,
        const QString &action,
        const QString &behavior,
        bool enabled);
    Q_INVOKABLE bool saveActiveProfile();
    Q_INVOKABLE bool reloadActiveProfile();
    Q_INVOKABLE bool duplicateActiveProfile(const QString &newName);
    Q_INVOKABLE bool deleteProfile(const QString &name);

signals:
    void stateChanged();
    void devicesChanged();
    void settingsChanged();
    void mappingChanged();
    void recordingsChanged();
    void recordingQualityChanged();

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
        QString gestureModelPath;

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
    void scheduleCameraRecovery(const QByteArray &deviceId);
    void cancelCameraRecovery();
    void pollCameraRecovery();

    QString mediaPipeLibraryPath() const;
    QString handLandmarkerModelPath() const;
    QString gestureModelPath() const;

    void clearTracking();
    void resetContinuousControl();
    void resetGestureActions();
    void updateContinuousControl(
        const TrackingFrame &frame,
        const GestureEventFrame &events);

    void initializeProfiles();
    bool loadProfile(const QString &name);
    void applyActiveMappings();
    void refreshOutputStatus();
    void updateCursorGeometry();

    void applyTrackingFrame(
        const TrackingFrame &frame,
        const GestureFrame &gestures,
        std::uint64_t generation);

    void updateGestureBackendStatus(
        const QString &backendName,
        const QString &modelStatus,
        bool modelActive,
        std::uint64_t generation);

    void workerLoop(
        std::stop_token stopToken);

    void updateStats();
    void tickGestureReplay();

    static QVariantList toVariantLandmarks(
        const HandTrackingState &hand);

    QMediaDevices mediaDevices_;
    QTimer cameraRecoveryTimer_;
    CameraReconnectPolicy cameraReconnectPolicy_;
    bool autoRecoverCamera_{true};
    QString cameraRecoveryStatus_;
    std::int64_t recoveryNotBeforeUs_{};
    int recoveryAttempt_{};
    std::int64_t sessionStartedUs_{};
    bool captureStallHandled_{};
    QList<QCameraDevice> cameras_;
    QList<QCameraFormat> formats_;

    std::unique_ptr<QCamera> camera_;
    // The capture sink is always alive, even with no QML page visible. This
    // prevents navigation/minimization from stopping MediaPipe or recording.
    // Declare before captureSession_ so the session is destroyed first.
    QVideoSink captureSink_;
    QMediaCaptureSession captureSession_;

    // Preview sinks are attached only while their view is actually active.
    // Never route more than one live preview at a time.
    QPointer<QVideoSink> controllerSink_;
    QPointer<QVideoSink> pipSink_;
    QPointer<QVideoSink> recordingSink_;
    bool pipActive_{false};
    bool mainWindowMinimized_{false};
    // Capture producer writes to a single coalescing presentation mailbox.
    // Only the GUI timer touches QVideoSink::setVideoFrame(). Neither capture
    // nor gesture recognition can be delayed by queued preview callbacks.
    LatestValueMailbox<QVideoFrame> previewMailbox_;
    std::atomic_bool previewCaptureEnabled_{false};
    QPointer<QVideoSink> activePreviewSink_;
    std::uint64_t previewGeneration_{0};
    void updatePreviewConnections();
    QString validatedRecordingPath(const QString &name) const;
    void setRecordingsError(const QString &message);

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

    QTimer previewTimer_;
    QTimer statsTimer_;
    QTimer replayTimer_;
    GestureReplayReader replayReader_;
    GestureEngine replayEngine_;
    GestureStateManager replayStateManager_;
    int replayFrames_{};
    QString replayStatus_;
    QString replayError_;
    std::array<QString, kHandCount> replayGestures_{QStringLiteral("NONE"), QStringLiteral("NONE")};
    std::array<QString, kHandCount> replayLabels_{QStringLiteral("UNLABELED"), QStringLiteral("UNLABELED")};
    bool pendingModelReload_{};
    bool reloadQueued_{};
    RuntimeMetricsSnapshot metricsSnapshot_;
    std::int64_t lastTrackingUiNotifyUs_{};
    std::int64_t lastDiagnosticsNotifyUs_{};

    std::atomic_bool swapHandedness_{};
    std::atomic_int recognitionThreshold_{80};
    std::array<std::atomic_int, kGestureClassCount> gestureThresholdOverrides_{};

    GestureStateManager gestureStateManager_;
    GestureCalibrator calibrator_;
    HandRangeCalibrator rangeCalibrator_;
    std::array<float, kHandCount> appliedMotionX_{1.0F, 1.0F};
    std::array<float, kHandCount> appliedMotionY_{1.0F, 1.0F};
    std::array<std::atomic<float>, kHandCount> appliedPinchLow_{};
    std::array<std::atomic<float>, kHandCount> appliedPinchHigh_{};
    CursorPlayground cursorPlayground_;
    bool playgroundMode_{}; // also remains true after six targets are completed
    std::int64_t playgroundSampleUs_{};
    ActionMapper actionMapper_;
    std::unique_ptr<ProfileStore> profileStore_;
    MappingProfile activeMappingProfile_;
    QStringList mappingProfileNames_;
    QString activeProfile_{QStringLiteral("Default")};
    QString mappingError_;

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

    int debounceMs_{120};
    int cooldownMs_{250};
    bool requireRelease_{true};
    int gestureGraceMs_{100};
    int gestureReleaseThreshold_{55};
    QVariantList gestureDiagnostics_;
    GestureDataRecorder recorder_;
    std::int64_t lastRecordingQualitySignalUs_{};
    QString recordingPath_;
    QString recordingError_;
    QString recordingsError_;

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

    QString leftGesture_{QStringLiteral("NONE")};
    QString rightGesture_{QStringLiteral("NONE")};
    double leftGestureConfidence_{};
    double rightGestureConfidence_{};
    QString leftGestureEvent_{QStringLiteral("IDLE")};
    QString rightGestureEvent_{QStringLiteral("IDLE")};
    QString gestureBackendName_{QStringLiteral("Rules")};
    QString gestureModelStatus_{QStringLiteral("Rule fallback active")};
    bool gestureModelActive_{};
};

} // namespace vc
