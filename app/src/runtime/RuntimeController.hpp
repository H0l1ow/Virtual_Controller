#pragma once

#include "tracking/TrackingTypes.hpp"

#include <QCamera>
#include <QCameraDevice>
#include <QCameraFormat>
#include <QElapsedTimer>
#include <QMediaCaptureSession>
#include <QMediaDevices>
#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariant>
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

struct VideoPacket
{
    QVideoFrame frame;
    std::int64_t captureUs{};
    std::uint64_t sequence{};
};


// Single-slot mailbox.
//
// If inference is slower than the camera, the old waiting frame is replaced
// by the newest one. This prevents an increasing latency queue.
class LatestFrameSlot final
{
public:
    void publish(VideoPacket packet);

    std::optional<VideoPacket> take(
        std::chrono::milliseconds timeout);

    void close();

    bool isClosed();

private:
    std::mutex mutex_;
    std::condition_variable condition_;

    std::optional<VideoPacket> packet_;

    bool closed_{};
};


class RuntimeController final : public QObject
{
    Q_OBJECT

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

    Q_PROPERTY(
        int fps
            READ fps
                NOTIFY stateChanged)

    Q_PROPERTY(
        QString resolution
            READ resolution
                NOTIFY stateChanged)

    Q_PROPERTY(
        int latencyMs
            READ latencyMs
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

public:
    explicit RuntimeController(
        QObject *parent = nullptr);

    ~RuntimeController() override;

    // Final application shutdown is different from the normal Stop action.
    //
    // stop() only stops the current camera/tracking session and deliberately
    // keeps the persistent worker alive so tracking can be started again.
    // shutdownForExit() also asks that worker to terminate and waits for a
    // bounded amount of time.  Returning false means a native MediaPipe/TFLite
    // call did not return and the caller must not destroy RuntimeController,
    // because the worker may still be executing code that references it.
    bool shutdownForExit(
        std::chrono::milliseconds timeout);


    bool cameraRunning() const
    {
        return cameraRunning_;
    }

    bool pipelineRunning() const
    {
        return pipelineRunning_;
    }

    bool trackerReady() const
    {
        return trackerReady_;
    }


    QStringList cameraNames() const
    {
        QStringList names;

        names.reserve(cameras_.size());

        for (const auto &camera : cameras_) {
            names.append(camera.description());
        }

        return names;
    }

    QStringList formatNames() const;


    int cameraIndex() const
    {
        return cameraIndex_;
    }

    int formatIndex() const
    {
        return formatIndex_;
    }


    int fps() const
    {
        return fps_;
    }

    QString resolution() const
    {
        return resolution_;
    }

    int latencyMs() const
    {
        return latencyMs_;
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
        return leftConfidence_;
    }

    double rightConfidence() const
    {
        return rightConfidence_;
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


    void setDetectionConfidence(int value);
    void setTrackingConfidence(int value);
    void setSwapHandedness(bool value);


    Q_INVOKABLE void attachVideoOutput(QObject *output);

    Q_INVOKABLE void selectCamera(int index);
    Q_INVOKABLE void selectFormat(int index);

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void togglePipeline();

    // Requested by the QML close handler. The actual process-exit policy is
    // owned by main.cpp so RuntimeController does not need platform-specific
    // process APIs.
    Q_INVOKABLE void requestApplicationExit();


signals:
    void stateChanged();
    void devicesChanged();
    void settingsChanged();
    void applicationExitRequested();


private:
    struct WorkerSession
    {
        std::shared_ptr<LatestFrameSlot> inbox;

        QString libraryPath;
        QString modelPath;

        float detectionConfidence{};
        float trackingConfidence{};

        std::uint64_t generation{};
    };

    // Lives independently from RuntimeController so final shutdown can wait
    // until workerLoop() has returned without using std::thread::join().
    struct WorkerExitState
    {
        std::mutex mutex;
        std::condition_variable condition;
        bool exited{};
    };


    void refreshDevices();
    void refreshFormats();


    QString mediaPipeLibraryPath() const;
    QString handLandmarkerModelPath() const;


    void clearTracking();


    void applyTrackingFrame(
        const TrackingFrame &frame,
        std::uint64_t generation);


    // Persistent worker.
    //
    // The system thread is created once in RuntimeController's constructor.
    // Start/Stop only create and destroy MediaPipe tracking sessions.
    void workerLoop(
        std::stop_token stopToken);


    void updateStats();


    static QVariantList toVariantLandmarks(
        const HandTrackingState &hand);


    // ------------------------------------------------------------------------
    // Qt Multimedia
    // ------------------------------------------------------------------------

    QMediaDevices mediaDevices_;

    QList<QCameraDevice> cameras_;
    QList<QCameraFormat> formats_;

    std::unique_ptr<QCamera> camera_;

    QMediaCaptureSession captureSession_;

    QPointer<QObject> videoOutput_;


    QMetaObject::Connection frameConnection_;
    QMetaObject::Connection cameraActiveConnection_;
    QMetaObject::Connection cameraErrorConnection_;


    // ------------------------------------------------------------------------
    // Runtime frame path
    // ------------------------------------------------------------------------

    std::shared_ptr<LatestFrameSlot> inbox_;


    // ------------------------------------------------------------------------
    // Persistent tracking worker
    // ------------------------------------------------------------------------

    std::mutex workerMutex_;
    std::condition_variable workerCondition_;

    std::optional<WorkerSession> pendingWorkerSession_;

    std::shared_ptr<WorkerExitState> workerExitState_;
    std::jthread worker_;


    // ------------------------------------------------------------------------
    // Statistics
    // ------------------------------------------------------------------------

    QTimer statsTimer_;
    QElapsedTimer statsClock_;

    std::atomic<std::uint64_t> receivedFrames_{};
    std::atomic<std::uint64_t> processedFrames_{};

    std::atomic<std::int64_t> lastFrameReceivedUs_{};

    // Worker uses this to make sure old sessions cannot affect statistics
    // after a rapid Stop -> Start sequence.
    std::atomic<std::uint64_t> activeGeneration_{};


    // ------------------------------------------------------------------------
    // Runtime settings shared with worker
    // ------------------------------------------------------------------------

    std::atomic_bool swapHandedness_{};


    // ------------------------------------------------------------------------
    // GUI-thread state
    // ------------------------------------------------------------------------

    std::uint64_t generation_{};
    std::uint64_t lastFpsSampleCount_{};

    int cameraIndex_{};
    int formatIndex_{};

    int detectionConfidence_{60};
    int trackingConfidence_{55};

    bool cameraRunning_{};
    bool pipelineRunning_{};
    bool trackerReady_{};

    int fps_{};
    int latencyMs_{};

    QString resolution_{"-"};
    QString errorMessage_;

    bool leftTracked_{};
    bool rightTracked_{};

    double leftConfidence_{};
    double rightConfidence_{};

    QVariantList leftLandmarks_;
    QVariantList rightLandmarks_;
};

} // namespace vc