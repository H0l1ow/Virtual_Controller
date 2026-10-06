#include "runtime/RuntimeController.hpp"

#include "tracking/MediaPipeTracker.hpp"

#include <QCoreApplication>
#include <QFileInfo>
#include <QImage>
#include <QDebug>
#include <QMetaObject>
#include <QVariantMap>
#include <QVideoSink>
#include <QtGlobal>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <exception>
#include <stdexcept>
#include <utility>
#include <vector>

namespace vc {


// ============================================================================
// LatestFrameSlot
// ============================================================================

void LatestFrameSlot::publish(
    VideoPacket packet)
{
    {
        std::lock_guard lock(mutex_);

        if (closed_) {
            return;
        }

        // Replace the previous waiting frame.
        //
        // We intentionally do not build a frame queue because for interactive
        // control the newest frame is more useful than an old frame.
        packet_ =
            std::move(packet);
    }

    condition_.notify_one();
}


std::optional<VideoPacket>
LatestFrameSlot::take(
    std::chrono::milliseconds timeout)
{
    std::unique_lock lock(mutex_);

    condition_.wait_for(
        lock,
        timeout,
        [this] {
            return closed_
                   || packet_.has_value();
        });


    if (!packet_) {
        return std::nullopt;
    }


    auto result =
        std::move(packet_);

    packet_.reset();

    return result;
}


void LatestFrameSlot::close()
{
    {
        std::lock_guard lock(mutex_);

        if (closed_) {
            return;
        }

        closed_ = true;

        // Do not allow a stale frame to be processed after STOP.
        packet_.reset();
    }

    condition_.notify_all();
}


bool LatestFrameSlot::isClosed()
{
    std::lock_guard lock(mutex_);

    return closed_;
}


// ============================================================================
// RuntimeController
// ============================================================================

RuntimeController::RuntimeController(
    QObject *parent)
    : QObject(parent)
    , mediaDevices_(this)
    , captureSession_(this)
    , statsTimer_(this)
{
    refreshDevices();


    connect(
        &mediaDevices_,
        &QMediaDevices::videoInputsChanged,
        this,
        [this] {
            if (pipelineRunning_) {
                stop();
            }

            refreshDevices();
        });


    statsTimer_.setInterval(500);


    connect(
        &statsTimer_,
        &QTimer::timeout,
        this,
        &RuntimeController::updateStats);


    // --------------------------------------------------------------------
    // Create exactly one system worker thread.
    //
    // It remains alive for the entire RuntimeController lifetime.
    //
    // Start/Stop will create and destroy MediaPipeTracker instances inside
    // this thread, but will NOT create/destroy the system thread itself.
    // --------------------------------------------------------------------

    workerExitState_ =
        std::make_shared<WorkerExitState>();

    const auto workerExitState =
        workerExitState_;

    worker_ =
        std::jthread(
            [this, workerExitState](
                std::stop_token stopToken) {

                workerLoop(
                    stopToken);

                // From this point onward the native worker thread no longer
                // accesses RuntimeController. Signal through an object whose
                // lifetime is independent from RuntimeController itself.
                {
                    std::lock_guard lock(
                        workerExitState->mutex);

                    workerExitState->exited = true;
                }

                workerExitState
                    ->condition
                    .notify_all();
            });
}


RuntimeController::~RuntimeController()
{
    // main() performs the bounded shutdown before normal object destruction.
    // In the regular path worker_ is already detached here and this call is
    // effectively a no-op.  Keep a long fallback for non-main owners rather
    // than reintroducing an unbounded destructor wait.
    if (!shutdownForExit(
            std::chrono::seconds(5))) {

        // It is unsafe to continue destroying this QObject while workerLoop()
        // may still be using `this`.  The process-level shutdown path in
        // main.cpp prevents reaching this branch during normal application
        // exit.  If a future owner destroys RuntimeController elsewhere,
        // terminate rather than returning into a use-after-free.
        std::terminate();
    }
}


bool RuntimeController::shutdownForExit(
    std::chrono::milliseconds timeout)
{
    qInfo() << "[shutdown] shutdownForExit begin";

    // First stop camera + current tracking session. stop() intentionally does
    // not wait for MediaPipe, so it remains suitable for the normal Stop UI.
    stop();


    // Cancel a session that has not yet been picked up by the worker.
    {
        std::lock_guard lock(
            workerMutex_);

        if (pendingWorkerSession_) {

            if (pendingWorkerSession_->inbox) {
                pendingWorkerSession_
                    ->inbox
                    ->close();
            }

            pendingWorkerSession_.reset();
        }
    }


    // Disconnect the QML VideoOutput while the QML engine is still alive.
    captureSession_.setVideoOutput(
        nullptr);


    if (!worker_.joinable()) {
        qInfo() << "[shutdown] worker already not joinable";
        return true;
    }


    worker_.request_stop();
    workerCondition_.notify_all();


    const auto workerExitState =
        workerExitState_;


    bool exited = false;

    {
        std::unique_lock lock(
            workerExitState->mutex);

        exited =
            workerExitState
                ->condition
                .wait_for(
                    lock,
                    timeout,
                    [&workerExitState] {
                        return workerExitState->exited;
                    });
    }


    if (!exited) {
        qWarning() << "[shutdown] worker exit timeout";
        return false;
    }


    // The latch is set only after workerLoop() has returned, so the worker no
    // longer accesses RuntimeController.  Do not join here: some Windows ML
    // runtimes can still spend an unbounded amount of time in native/TLS
    // thread teardown after the C++ worker function has logically finished.
    worker_.detach();

    qInfo() << "[shutdown] shutdownForExit end";
    return true;
}


// ============================================================================
// Camera / format names
// ============================================================================

QStringList RuntimeController::formatNames() const
{
    QStringList names;

    names.reserve(
        formats_.size());


    for (const auto &format
         : formats_) {

        const auto size =
            format.resolution();


        names.append(
            QStringLiteral(
                "%1 x %2 @ %3 FPS")
                .arg(
                    size.width())
                .arg(
                    size.height())
                .arg(
                    qRound(
                        format.maxFrameRate())));
    }


    return names;
}


// ============================================================================
// Tracking status
// ============================================================================

QString RuntimeController::trackingStatus() const
{
    if (!pipelineRunning_) {
        return QStringLiteral(
            "Stopped");
    }


    if (!errorMessage_.isEmpty()) {
        return QStringLiteral(
            "Error");
    }


    if (!trackerReady_) {
        return QStringLiteral(
            "Starting");
    }


    const auto lastFrameUs =
        lastFrameReceivedUs_.load();


    if (lastFrameUs > 0
        && nowUs() - lastFrameUs
               > 500000) {

        return QStringLiteral(
            "No frames");
    }


    if (leftTracked_
        && rightTracked_) {

        return QStringLiteral(
            "Stable");
    }


    if (leftTracked_
        || rightTracked_) {

        return QStringLiteral(
            "Degraded");
    }


    return QStringLiteral(
        "No hands");
}


// ============================================================================
// Settings
// ============================================================================

void RuntimeController::setDetectionConfidence(
    int value)
{
    value =
        std::clamp(
            value,
            0,
            100);


    if (detectionConfidence_
        == value) {

        return;
    }


    detectionConfidence_ =
        value;


    emit settingsChanged();
}


void RuntimeController::setTrackingConfidence(
    int value)
{
    value =
        std::clamp(
            value,
            0,
            100);


    if (trackingConfidence_
        == value) {

        return;
    }


    trackingConfidence_ =
        value;


    emit settingsChanged();
}


void RuntimeController::setSwapHandedness(
    bool value)
{
    if (swapHandedness_.exchange(
            value)
        == value) {

        return;
    }


    emit settingsChanged();
}


// ============================================================================
// Devices
// ============================================================================

void RuntimeController::refreshDevices()
{
    cameras_ =
        QMediaDevices::videoInputs();


    if (cameras_.isEmpty()) {

        cameraIndex_ = 0;
        formatIndex_ = 0;

        formats_.clear();

        resolution_ =
            QStringLiteral("-");


        emit devicesChanged();
        emit stateChanged();

        return;
    }


    cameraIndex_ =
        std::clamp(
            cameraIndex_,
            0,
            static_cast<int>(
                cameras_.size())
                - 1);


    refreshFormats();


    emit devicesChanged();
}


void RuntimeController::refreshFormats()
{
    formats_.clear();

    formatIndex_ = 0;


    if (cameras_.isEmpty()) {

        resolution_ =
            QStringLiteral("-");

        return;
    }


    formats_ =
        cameras_[cameraIndex_]
            .videoFormats();


    // Prefer a practical tracking default:
    // approximately 1280x720 @ 30 FPS.
    std::sort(
        formats_.begin(),
        formats_.end(),
        [](
            const QCameraFormat &a,
            const QCameraFormat &b) {

            const auto score =
                [](
                    const QCameraFormat &format) {

                    const auto size =
                        format.resolution();


                    const int resolutionPenalty =
                        std::abs(
                            size.width()
                            - 1280)
                        +
                        std::abs(
                            size.height()
                            - 720);


                    const int fpsPenalty =
                        qRound(
                            std::abs(
                                format.maxFrameRate()
                                - 30.0F)
                            * 12.0F);


                    return resolutionPenalty
                           + fpsPenalty;
                };


            return score(a)
                   < score(b);
        });


    if (!formats_.isEmpty()) {

        const auto size =
            formats_.front()
                .resolution();


        resolution_ =
            QStringLiteral(
                "%1 x %2")
                .arg(
                    size.width())
                .arg(
                    size.height());

    } else {

        resolution_ =
            QStringLiteral(
                "Default");
    }
}


// ============================================================================
// VideoOutput
// ============================================================================

void RuntimeController::attachVideoOutput(
    QObject *output)
{
    if (!output) {

        if (pipelineRunning_) {
            stop();
        }


        videoOutput_.clear();


        captureSession_.setVideoOutput(
            nullptr);


        return;
    }


    if (pipelineRunning_
        && videoOutput_ != output) {

        errorMessage_ =
            QStringLiteral(
                "Stop tracking before replacing "
                "the video output.");


        emit stateChanged();

        return;
    }


    videoOutput_ =
        output;


    captureSession_.setVideoOutput(
        output);
}


// ============================================================================
// Camera selection
// ============================================================================

void RuntimeController::selectCamera(
    int index)
{
    if (pipelineRunning_
        || index < 0
        || index
               >= static_cast<int>(
                   cameras_.size())) {

        return;
    }


    if (cameraIndex_
        == index) {

        return;
    }


    cameraIndex_ =
        index;


    refreshFormats();


    emit devicesChanged();
    emit stateChanged();
}


void RuntimeController::selectFormat(
    int index)
{
    if (pipelineRunning_
        || index < 0
        || index
               >= static_cast<int>(
                   formats_.size())) {

        return;
    }


    if (formatIndex_
        == index) {

        return;
    }


    formatIndex_ =
        index;


    const auto size =
        formats_[formatIndex_]
            .resolution();


    resolution_ =
        QStringLiteral(
            "%1 x %2")
            .arg(
                size.width())
            .arg(
                size.height());


    emit devicesChanged();
    emit stateChanged();
}


// ============================================================================
// MediaPipe paths
// ============================================================================

QString RuntimeController::mediaPipeLibraryPath() const
{
#ifdef _WIN32

    constexpr auto libraryName =
        "libmediapipe.dll";

#elif defined(__APPLE__)

    constexpr auto libraryName =
        "libmediapipe.dylib";

#else

    constexpr auto libraryName =
        "libmediapipe.so";

#endif


    const QString besideExecutable =
        QCoreApplication::applicationDirPath()
        + QStringLiteral(
            "/native/")
        + QString::fromLatin1(
            libraryName);


    if (QFileInfo::exists(
            besideExecutable)) {

        return besideExecutable;
    }


    const QString configured =
        QString::fromUtf8(
            VC_MEDIAPIPE_ROOT_PATH)
        + QLatin1Char('/')
        + QString::fromLatin1(
            libraryName);


    if (QFileInfo::exists(
            configured)) {

        return configured;
    }


    return
        QString::fromUtf8(
            VC_SOURCE_ROOT)
        + QStringLiteral(
            "/deps/mediapipe/")
        + QString::fromLatin1(
            libraryName);
}


QString RuntimeController::handLandmarkerModelPath() const
{
    const QString besideExecutable =
        QCoreApplication::applicationDirPath()
        + QStringLiteral(
            "/models/hand_landmarker.task");


    if (QFileInfo::exists(
            besideExecutable)) {

        return besideExecutable;
    }


    const QString configured =
        QString::fromUtf8(
            VC_HAND_LANDMARKER_MODEL_PATH);


    if (QFileInfo::exists(
            configured)) {

        return configured;
    }


    return
        QString::fromUtf8(
            VC_SOURCE_ROOT)
        + QStringLiteral(
            "/models/hand_landmarker.task");
}


// ============================================================================
// START
// ============================================================================

void RuntimeController::start()
{
    if (pipelineRunning_) {
        return;
    }


    errorMessage_.clear();


    // --------------------------------------------------------------------
    // Validate dependencies before changing runtime state.
    // --------------------------------------------------------------------

    if (cameras_.isEmpty()) {

        errorMessage_ =
            QStringLiteral(
                "No camera detected. "
                "Check Windows camera permissions.");


        emit stateChanged();

        return;
    }


    if (!videoOutput_) {

        errorMessage_ =
            QStringLiteral(
                "Camera preview is not attached yet.");


        emit stateChanged();

        return;
    }


    const QString libraryPath =
        mediaPipeLibraryPath();


    const QString modelPath =
        handLandmarkerModelPath();


    if (!QFileInfo::exists(
            libraryPath)) {

        errorMessage_ =
            QStringLiteral(
                "Missing MediaPipe runtime: %1")
                .arg(
                    libraryPath);


        emit stateChanged();

        return;
    }


    if (!QFileInfo::exists(
            modelPath)) {

        errorMessage_ =
            QStringLiteral(
                "Missing hand_landmarker.task: %1")
                .arg(
                    modelPath);


        emit stateChanged();

        return;
    }


    auto *videoSink =
        captureSession_.videoSink();


    if (!videoSink) {

        errorMessage_ =
            QStringLiteral(
                "VideoOutput did not expose "
                "a QVideoSink.");


        emit stateChanged();

        return;
    }


    // --------------------------------------------------------------------
    // Prepare GUI/runtime state.
    // --------------------------------------------------------------------

    clearTracking();


    trackerReady_ = false;
    cameraRunning_ = false;
    pipelineRunning_ = true;


    fps_ = 0;
    latencyMs_ = 0;


    receivedFrames_.store(0);
    processedFrames_.store(0);
    lastFrameReceivedUs_.store(0);


    lastFpsSampleCount_ = 0;


    statsClock_.restart();


    const std::uint64_t generation =
        ++generation_;


    activeGeneration_.store(
        generation);


    // Every tracking session gets a fresh mailbox.
    //
    // close() is intentionally terminal for LatestFrameSlot, therefore a
    // new slot is created on every Start.
    inbox_ =
        std::make_shared<
            LatestFrameSlot>();


    std::weak_ptr<LatestFrameSlot>
        weakInbox =
        inbox_;


    // --------------------------------------------------------------------
    // Camera frame callback.
    //
    // Do not perform frame conversion or MediaPipe inference here.
    // --------------------------------------------------------------------

    frameConnection_ =
        connect(
            videoSink,
            &QVideoSink::videoFrameChanged,
            this,
            [this, weakInbox](
                const QVideoFrame &frame) {

                if (!frame.isValid()) {
                    return;
                }


                const auto inbox =
                    weakInbox.lock();


                if (!inbox) {
                    return;
                }


                if (inbox->isClosed()) {
                    return;
                }


                const auto captureUs =
                    nowUs();


                const auto sequence =
                    receivedFrames_
                        .fetch_add(1)
                    + 1;


                lastFrameReceivedUs_
                    .store(
                        captureUs);


                inbox->publish({
                    frame,
                    captureUs,
                    sequence
                });
            },
            Qt::DirectConnection);


    // --------------------------------------------------------------------
    // Camera
    // --------------------------------------------------------------------

    camera_ =
        std::make_unique<QCamera>(
            cameras_[cameraIndex_]);


    if (!formats_.isEmpty()) {

        camera_->setCameraFormat(
            formats_[formatIndex_]);
    }


    captureSession_.setCamera(
        camera_.get());


    cameraActiveConnection_ =
        connect(
            camera_.get(),
            &QCamera::activeChanged,
            this,
            [this, generation](
                bool active) {

                if (generation
                    != generation_) {

                    return;
                }


                cameraRunning_ =
                    active;


                emit stateChanged();
            });


    cameraErrorConnection_ =
        connect(
            camera_.get(),
            &QCamera::errorOccurred,
            this,
            [this, generation](
                QCamera::Error error,
                const QString &message) {

                if (generation
                        != generation_
                    || error
                           == QCamera::NoError) {

                    return;
                }


                errorMessage_ =
                    message.isEmpty()
                        ? QStringLiteral(
                              "Camera error")
                        : message;


                emit stateChanged();


                // Never destroy the camera directly from its own signal.
                QMetaObject::invokeMethod(
                    this,
                    [this, generation] {

                        if (generation
                            == generation_) {

                            stop();
                        }
                    },
                    Qt::QueuedConnection);
            });


    // --------------------------------------------------------------------
    // Submit tracking session to the persistent worker.
    // --------------------------------------------------------------------

    WorkerSession session;

    session.inbox =
        inbox_;

    session.libraryPath =
        libraryPath;

    session.modelPath =
        modelPath;

    session.detectionConfidence =
        static_cast<float>(
            detectionConfidence_)
        / 100.0F;

    session.trackingConfidence =
        static_cast<float>(
            trackingConfidence_)
        / 100.0F;

    session.generation =
        generation;


    {
        std::lock_guard lock(
            workerMutex_);


        // Normally there should be no pending session here.
        //
        // Still, if the user performs an extremely fast Stop -> Start ->
        // Start sequence, replacing a not-yet-started request is safer than
        // starting an obsolete tracker.
        if (pendingWorkerSession_) {

            if (pendingWorkerSession_
                    ->inbox) {

                pendingWorkerSession_
                    ->inbox
                    ->close();
            }


            pendingWorkerSession_
                .reset();
        }


        pendingWorkerSession_ =
            std::move(session);
    }


    workerCondition_
        .notify_one();


    // Start camera after the worker request has been submitted.
    //
    // It is fine if frames arrive while MediaPipe is initializing:
    // LatestFrameSlot will retain only the newest one.
    camera_->start();


    statsTimer_.start();


    emit stateChanged();
}


// ============================================================================
// STOP
// ============================================================================

void RuntimeController::stop()
{
    qInfo() << "[shutdown] RuntimeController::stop begin";

    // --------------------------------------------------------------------
    // Invalidate all queued results from the previous session immediately.
    // --------------------------------------------------------------------

    ++generation_;

    activeGeneration_.store(0);

    pipelineRunning_ = false;
    trackerReady_ = false;


    // --------------------------------------------------------------------
    // Stop handing Qt Multimedia buffers to the worker FIRST.
    //
    // A QVideoFrame may reference a backend-owned Media Foundation buffer.
    // The previous implementation stopped/destroyed QCamera before closing
    // the mailbox, so a queued frame could still pin that backend resource
    // during camera teardown.
    // --------------------------------------------------------------------

    disconnect(
        frameConnection_);


    const auto stoppingInbox =
        inbox_;


    if (stoppingInbox) {
        stoppingInbox->close();
    }


    {
        std::lock_guard lock(
            workerMutex_);


        // If the worker has not picked this session up yet, remove the
        // pending request completely.
        if (pendingWorkerSession_
            && pendingWorkerSession_->inbox
                   == stoppingInbox) {

            pendingWorkerSession_
                ->inbox
                ->close();

            pendingWorkerSession_
                .reset();
        }
    }


    workerCondition_
        .notify_all();


    inbox_.reset();


    // --------------------------------------------------------------------
    // Now release the camera backend.
    // --------------------------------------------------------------------

    disconnect(
        cameraActiveConnection_);

    disconnect(
        cameraErrorConnection_);


    if (camera_) {
        qInfo() << "[shutdown] stopping QCamera";
        camera_->stop();
        qInfo() << "[shutdown] QCamera::stop returned";
    }


    qInfo() << "[shutdown] detaching QCamera from capture session";
    captureSession_.setCamera(
        nullptr);
    qInfo() << "[shutdown] QMediaCaptureSession::setCamera(nullptr) returned";


    camera_.reset();
    qInfo() << "[shutdown] QCamera destroyed";


    cameraRunning_ = false;


    // --------------------------------------------------------------------
    // Reset public runtime state immediately.
    // --------------------------------------------------------------------

    statsTimer_.stop();


    fps_ = 0;
    latencyMs_ = 0;


    lastFrameReceivedUs_
        .store(0);


    clearTracking();


    emit stateChanged();

    qInfo() << "[shutdown] RuntimeController::stop end";
}


// ============================================================================
// Toggle
// ============================================================================

void RuntimeController::togglePipeline()
{
    if (pipelineRunning_) {
        stop();
    } else {
        start();
    }
}


void RuntimeController::requestApplicationExit()
{
    qInfo() << "[shutdown] application exit requested from QML";
    emit applicationExitRequested();
}


// ============================================================================
// Persistent worker
// ============================================================================

void RuntimeController::workerLoop(
    std::stop_token stopToken)
{
    while (!stopToken.stop_requested()) {

        std::optional<WorkerSession>
            session;


        // ----------------------------------------------------------------
        // IDLE state.
        //
        // The OS thread remains alive here between Start/Stop cycles.
        // ----------------------------------------------------------------

        {
            std::unique_lock lock(
                workerMutex_);


            workerCondition_.wait(
                lock,
                [this, &stopToken] {

                    return
                        stopToken
                            .stop_requested()
                        ||
                        pendingWorkerSession_
                            .has_value();
                });


            if (stopToken
                    .stop_requested()) {

                break;
            }


            session =
                std::move(
                    pendingWorkerSession_);


            pendingWorkerSession_
                .reset();
        }


        if (!session
            || !session->inbox) {

            continue;
        }


        // The user may have pressed STOP before the worker even managed to
        // create MediaPipeTracker.
        if (session->inbox
                ->isClosed()) {

            continue;
        }


        const std::uint64_t generation =
            session->generation;


        try {

            // ------------------------------------------------------------
            // TRACKING SESSION begins.
            //
            // MediaPipeTracker is created and destroyed on this same
            // persistent worker thread.
            // ------------------------------------------------------------

            MediaPipeTracker tracker(
                session->libraryPath,
                session->modelPath,
                session->detectionConfidence,
                session->trackingConfidence);


            // Inform GUI that MediaPipe initialization completed.
            QMetaObject::invokeMethod(
                this,
                [this, generation] {

                    if (generation
                            != generation_
                        || !pipelineRunning_) {

                        return;
                    }


                    trackerReady_ =
                        true;


                    emit stateChanged();
                },
                Qt::QueuedConnection);


            // ------------------------------------------------------------
            // PROCESSING LOOP
            // ------------------------------------------------------------

            while (
                !stopToken.stop_requested()
                && !session->inbox
                        ->isClosed()) {

                auto packet =
                    session->inbox
                        ->take(
                            std::chrono::milliseconds(
                                25));


                if (!packet) {
                    continue;
                }


                // STOP might have happened between take() and this point.
                if (stopToken
                        .stop_requested()
                    || session->inbox
                           ->isClosed()) {

                    break;
                }


                const auto captureUs =
                    packet->captureUs;

                const auto sequence =
                    packet->sequence;


                // Make an owned CPU copy and release QVideoFrame BEFORE
                // entering MediaPipe. QVideoFrame can hold a native camera
                // buffer, and retaining it during inference/teardown can keep
                // the Windows multimedia backend alive.
                QImage image =
                    packet->frame
                        .toImage()
                        .convertToFormat(
                            QImage::Format_RGB888)
                        .copy();


                packet.reset();


                if (image.isNull()) {

                    throw std::runtime_error(
                        "Cannot convert camera frame "
                        "to RGB888");
                }


                std::vector<std::uint8_t>
                    rgb(
                        static_cast<std::size_t>(
                            image.width())
                        *
                        static_cast<std::size_t>(
                            image.height())
                        *
                        3U);


                // QImage scanlines can contain padding.
                // MediaPipe gets tightly packed RGB888.
                for (int y = 0;
                     y < image.height();
                     ++y) {

                    std::memcpy(
                        rgb.data()
                            +
                            static_cast<
                                std::size_t>(y)
                                *
                                static_cast<
                                    std::size_t>(
                                    image.width())
                                *
                                3U,

                        image.constScanLine(
                            y),

                        static_cast<
                            std::size_t>(
                            image.width())
                            *
                            3U);
                }


                const TrackingFrame
                    tracking =
                    tracker.process(
                        image.width(),
                        image.height(),
                        rgb,
                        captureUs,
                        sequence,
                        swapHandedness_
                            .load());


                // Statistics only belong to the currently active session.
                //
                // An old tracker may finish its last inference slightly
                // after a rapid Stop -> Start sequence.
                if (activeGeneration_
                        .load()
                    == generation) {

                    processedFrames_
                        .fetch_add(1);
                }


                // Tracking result is copied to the GUI thread.
                QMetaObject::invokeMethod(
                    this,
                    [this,
                     tracking,
                     generation] {

                        applyTrackingFrame(
                            tracking,
                            generation);
                    },
                    Qt::QueuedConnection);
            }


            // ------------------------------------------------------------
            // Leaving this scope destroys MediaPipeTracker.
            //
            // We already confirmed through logging that
            // MpHandLandmarkerClose() completes correctly.
            //
            // The OS worker thread itself remains alive and returns to
            // IDLE instead of terminating.
            // ------------------------------------------------------------
        }
        catch (const std::exception &error) {

            const QString message =
                QString::fromUtf8(
                    error.what());


            // Only the currently active generation is allowed to report an
            // error to the GUI.
            QMetaObject::invokeMethod(
                this,
                [this,
                 message,
                 generation] {

                    if (generation
                            != generation_
                        || !pipelineRunning_) {

                        return;
                    }


                    errorMessage_ =
                        message;


                    emit stateChanged();


                    // stop() no longer joins this worker, therefore calling
                    // it asynchronously from the GUI thread is safe.
                    stop();
                },
                Qt::QueuedConnection);
        }


        // Loop back to IDLE and wait for the next Start.
    }

    qInfo() << "[shutdown] workerLoop exited";
}


// ============================================================================
// Tracking result conversion
// ============================================================================

QVariantList RuntimeController::toVariantLandmarks(
    const HandTrackingState &hand)
{
    QVariantList result;


    if (!hand.tracked) {
        return result;
    }


    result.reserve(21);


    for (const auto &landmark
         : hand.landmarks) {

        QVariantMap point;


        point.insert(
            QStringLiteral("x"),
            landmark.x);

        point.insert(
            QStringLiteral("y"),
            landmark.y);

        point.insert(
            QStringLiteral("z"),
            landmark.z);


        result.append(
            point);
    }


    return result;
}


// ============================================================================
// Apply tracking result on GUI thread
// ============================================================================

void RuntimeController::applyTrackingFrame(
    const TrackingFrame &frame,
    std::uint64_t generation)
{
    // Results from a stopped or superseded tracking session are discarded.
    if (generation
            != generation_
        || !pipelineRunning_) {

        return;
    }


    leftTracked_ =
        frame.hands[0]
            .tracked;

    rightTracked_ =
        frame.hands[1]
            .tracked;


    leftConfidence_ =
        frame.hands[0]
            .confidence;

    rightConfidence_ =
        frame.hands[1]
            .confidence;


    leftLandmarks_ =
        toVariantLandmarks(
            frame.hands[0]);

    rightLandmarks_ =
        toVariantLandmarks(
            frame.hands[1]);


    latencyMs_ =
        static_cast<int>(
            std::max<
                std::int64_t>(
                0,
                nowUs()
                    - frame.captureUs)
            /
            1000);


    resolution_ =
        QStringLiteral(
            "%1 x %2")
            .arg(
                frame.width)
            .arg(
                frame.height);


    emit stateChanged();
}


// ============================================================================
// Tracking reset
// ============================================================================

void RuntimeController::clearTracking()
{
    leftTracked_ = false;
    rightTracked_ = false;


    leftConfidence_ = 0.0;
    rightConfidence_ = 0.0;


    leftLandmarks_.clear();
    rightLandmarks_.clear();
}


// ============================================================================
// Statistics
// ============================================================================

void RuntimeController::updateStats()
{
    if (!pipelineRunning_) {
        return;
    }


    const qint64 elapsedMs =
        statsClock_.restart();


    if (elapsedMs > 0) {

        const auto currentCount =
            processedFrames_
                .load();


        const auto delta =
            currentCount
            - lastFpsSampleCount_;


        lastFpsSampleCount_ =
            currentCount;


        fps_ =
            qRound(
                static_cast<double>(
                    delta)
                *
                1000.0
                /
                static_cast<double>(
                    elapsedMs));
    }


    const auto lastFrameUs =
        lastFrameReceivedUs_
            .load();


    // Do not leave an old hand visible when the camera stream disappears.
    if (lastFrameUs > 0
        && nowUs() - lastFrameUs
               > 500000) {

        clearTracking();

        latencyMs_ = 0;
    }


    emit stateChanged();
}

} // namespace vc