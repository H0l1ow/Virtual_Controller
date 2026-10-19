#include "runtime/RuntimeController.hpp"

#include "gestures/GestureEngine.hpp"
#include "gestures/OnnxGestureRecognizer.hpp"
#include "tracking/HandIdentityStabilizer.hpp"
#include "tracking/MediaPipeTracker.hpp"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QUrl>
#include <QDebug>
#include <QDateTime>
#include <QGuiApplication>
#include <QFileInfo>
#include <QImage>
#include <QMetaObject>
#include <QScreen>
#include <QStandardPaths>
#include <QVariantMap>
#include <QVideoFrameFormat>
#include <QVideoSink>
#include <QtGlobal>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <exception>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <optional>
#include <limits>

namespace vc {

namespace {

QString gestureDisplayName(GestureClass gesture)
{
    switch (gesture) {
    case GestureClass::Fist:
        return QStringLiteral("FIST");
    case GestureClass::OpenHand:
        return QStringLiteral("OPEN HAND");
    case GestureClass::Point:
        return QStringLiteral("POINT");
    case GestureClass::Pinch:
        return QStringLiteral("PINCH");
    case GestureClass::None:
    default:
        return QStringLiteral("NONE");
    }
}

QString gestureEventDisplayName(GestureEventPhase phase)
{
    return QString::fromLatin1(gestureEventPhaseName(phase));
}

QString filePathDisplay(const std::filesystem::path &path)
{
    const auto bytes = path.u8string();
    return QString::fromUtf8(reinterpret_cast<const char *>(bytes.data()),
                             static_cast<qsizetype>(bytes.size()));
}

QVariantMap diagnosticMap(const GesturePrediction &prediction,
                          const GestureHandDebugState &state)
{
    QVariantMap item;
    item.insert(QStringLiteral("reason"),
                QString::fromLatin1(gestureDecisionName(prediction.decision)));
    item.insert(QStringLiteral("raw"), gestureDisplayName(prediction.gesture));
    item.insert(QStringLiteral("candidate"), gestureDisplayName(state.candidate));
    item.insert(QStringLiteral("active"), gestureDisplayName(state.active));
    item.insert(QStringLiteral("inGrace"), state.inGracePeriod);
    item.insert(QStringLiteral("inCooldown"), state.inCooldown);
    item.insert(QStringLiteral("blocked"), state.waitingForRelease);
    item.insert(QStringLiteral("geometryValid"), prediction.geometryValid);
    item.insert(QStringLiteral("pinch"), prediction.pinchDistance);
    QVariantList scores;
    for (std::size_t i = 1; i < kGestureClassCount; ++i) {
        QVariantMap score;
        score.insert(QStringLiteral("name"),
                     QString::fromLatin1(kGestureCanonicalNames[i]));
        score.insert(QStringLiteral("value"), prediction.scores[i]);
        scores.append(score);
    }
    QVariantList curls;
    for (float curl : prediction.fingerCurl) {
        curls.append(curl);
    }
    item.insert(QStringLiteral("scores"), scores);
    item.insert(QStringLiteral("curl"), curls);
    return item;
}

QString actionDisplayName(LogicalAction action)
{
    switch (action) {
    case LogicalAction::MouseLeft: return QStringLiteral("Left click");
    case LogicalAction::MouseRight: return QStringLiteral("Right click");
    case LogicalAction::ScrollUp: return QStringLiteral("Scroll up");
    case LogicalAction::ScrollDown: return QStringLiteral("Scroll down");
    case LogicalAction::CursorFreeze: return QStringLiteral("Freeze cursor");
    case LogicalAction::CursorMoveEnable: return QStringLiteral("Enable cursor movement");
    case LogicalAction::KeySpace: return QStringLiteral("Key Space");
    case LogicalAction::KeyEnter: return QStringLiteral("Key Enter");
    case LogicalAction::KeyEscape: return QStringLiteral("Key Escape");
    case LogicalAction::KeyTab: return QStringLiteral("Key Tab");
    case LogicalAction::KeyLeft: return QStringLiteral("Key Left");
    case LogicalAction::KeyRight: return QStringLiteral("Key Right");
    case LogicalAction::KeyUp: return QStringLiteral("Key Up");
    case LogicalAction::KeyDown: return QStringLiteral("Key Down");
    case LogicalAction::KeyCtrl: return QStringLiteral("Key Ctrl");
    case LogicalAction::KeyShift: return QStringLiteral("Key Shift");
    case LogicalAction::KeyAlt: return QStringLiteral("Key Alt");
    default:
        break;
    }


    return QStringLiteral("Unknown");
}

QString actionOutputName(LogicalAction action)
{
    if (action == LogicalAction::CursorFreeze
        || action == LogicalAction::CursorMoveEnable) {
        return QStringLiteral("Control");
    }

    return action >= LogicalAction::KeySpace
        ? QStringLiteral("Keyboard")
        : QStringLiteral("Mouse");
}

std::optional<HandSide> parseHandName(const QString &value)
{
    if (value.compare(QStringLiteral("Left"), Qt::CaseInsensitive) == 0)
        return HandSide::Left;
    if (value.compare(QStringLiteral("Right"), Qt::CaseInsensitive) == 0)
        return HandSide::Right;
    return std::nullopt;
}

std::optional<GestureClass> parseGestureName(const QString &value)
{
    QString normalized = value.trimmed().toUpper();
    normalized.replace(' ', '_');
    for (std::size_t index = 0; index < kGestureCanonicalNames.size(); ++index) {
        if (normalized == QString::fromLatin1(kGestureCanonicalNames[index]))
            return static_cast<GestureClass>(index);
    }
    return std::nullopt;
}

std::optional<ActionBehavior> parseBehaviorName(const QString &value)
{
    if (value.compare(QStringLiteral("Press"), Qt::CaseInsensitive) == 0)
        return ActionBehavior::Press;
    if (value.compare(QStringLiteral("Hold"), Qt::CaseInsensitive) == 0)
        return ActionBehavior::Hold;
    if (value.compare(QStringLiteral("Toggle"), Qt::CaseInsensitive) == 0)
        return ActionBehavior::Toggle;
    return std::nullopt;
}

std::optional<LogicalAction> parseActionName(const QString &value)
{
    const QString normalized = value.trimmed();
    for (int index = static_cast<int>(LogicalAction::MouseLeft);
         index <= static_cast<int>(LogicalAction::KeyAlt);
         ++index) {
        const auto action = static_cast<LogicalAction>(index);
        if (normalized.compare(actionDisplayName(action), Qt::CaseInsensitive) == 0
            || normalized.compare(QString::fromLatin1(logicalActionId(action)), Qt::CaseInsensitive) == 0) {
            return action;
        }
    }
    return std::nullopt;
}

} // namespace

RuntimeController::RuntimeController(
    QObject *parent)
    : QObject(parent)
    , mediaDevices_(this)
    , cameraRecoveryTimer_(this)
    , captureSink_(this)
    , captureSession_(this)
    , previewTimer_(this)
    , statsTimer_(this)
{
    // Keep capture/MediaPipe independent of the visible Qt Quick VideoOutput.
    captureSession_.setVideoSink(&captureSink_);

    // Run this very small producer on the camera backend's emitting thread.
    // There is no per-frame Qt::QueuedConnection to GUI: old preview frames
    // get overwritten in one slot. Never access UI objects from this lambda.
    connect(&captureSink_, &QVideoSink::videoFrameChanged, this,
            [this](const QVideoFrame &frame) {
                if (previewCaptureEnabled_.load(std::memory_order_acquire)
                    && frame.isValid()) {
                    previewMailbox_.publish(frame);
                }
            }, Qt::DirectConnection);

    previewTimer_.setTimerType(Qt::PreciseTimer);
    connect(&previewTimer_, &QTimer::timeout, this, [this] {
        auto frame = previewMailbox_.take();
        if (activePreviewSink_ && frame && frame->isValid()) {
            // This method runs exclusively on the Qt GUI thread.
            activePreviewSink_->setVideoFrame(*frame);
        }
    });

    for (auto &threshold : gestureThresholdOverrides_)
        threshold.store(-1);
    for (auto &value : appliedPinchLow_) value.store(0.17F);
    for (auto &value : appliedPinchHigh_) value.store(0.43F);
    refreshDevices();
    updateCursorGeometry();
    cursorX_ = cursorScreenWidth_ / 2;
    cursorY_ = cursorScreenHeight_ / 2;
    cursorAccumulatorX_ = static_cast<double>(cursorX_);
    cursorAccumulatorY_ = static_cast<double>(cursorY_);

    initializeProfiles();

    cameraRecoveryTimer_.setInterval(1000);
    connect(&cameraRecoveryTimer_, &QTimer::timeout,
            this, &RuntimeController::pollCameraRecovery);

    connect(&mediaDevices_, &QMediaDevices::videoInputsChanged,
            this, [this] {
        // Device-list changes are not camera failures. Only stop the running
        // session if its *actual* selected device disappeared.
        if (camera_ && (runtimeState_ == RuntimeState::Running
                        || runtimeState_ == RuntimeState::Starting)) {
            const QByteArray selectedId = camera_->cameraDevice().id();
            const auto connected = QMediaDevices::videoInputs();
            const bool disappeared = std::none_of(connected.begin(), connected.end(),
                [&selectedId](const QCameraDevice &device) {
                    return device.id() == selectedId;
                });
            if (disappeared) {
                if (autoRecoverCamera_)
                    scheduleCameraRecovery(selectedId);
                requestSessionStop(RuntimeState::Stopped);
            }
        }
        refreshDevices();
    });

    replayTimer_.setInterval(33); // deterministic nominal 30 FPS; source timestamps are kept for recognition
    connect(&replayTimer_, &QTimer::timeout, this, &RuntimeController::tickGestureReplay);

    statsTimer_.setInterval(250);

    connect(
        &statsTimer_,
        &QTimer::timeout,
        this,
        &RuntimeController::updateStats);

    workerExitState_ =
        std::make_shared<WorkerExitState>();

    const auto workerExitState =
        workerExitState_;

    worker_ =
        std::jthread(
            [this, workerExitState](
                std::stop_token stopToken) {

                workerLoop(stopToken);

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
    if (!shutdownForExit(
            std::chrono::seconds(5))) {
        std::terminate();
    }
}

bool RuntimeController::shutdownForExit(
    std::chrono::milliseconds timeout)
{
    qInfo() << "[shutdown] shutdownForExit begin";

    stopGestureReplay();
    pendingModelReload_ = false;
    reloadQueued_ = false;
    outputService_.stop();
    refreshOutputStatus();
    resetContinuousControl();

    if (pipelineRunning()) {
        requestSessionStop(
            RuntimeState::Stopped);
    }

    {
        std::lock_guard lock(workerMutex_);

        if (pendingWorkerSession_) {
            if (pendingWorkerSession_->inbox) {
                pendingWorkerSession_
                    ->inbox
                    ->close();
            }

            pendingWorkerSession_.reset();
        }
    }

    previewCaptureEnabled_.store(false, std::memory_order_release);
    previewTimer_.stop();
    previewMailbox_.clear();
    if (controllerSink_) controllerSink_->setVideoFrame(QVideoFrame{});
    if (recordingSink_) recordingSink_->setVideoFrame(QVideoFrame{});
    if (pipSink_) pipSink_->setVideoFrame(QVideoFrame{});
    captureSession_.setVideoSink(nullptr);

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

    // The worker function has returned and no longer accesses this QObject.
    // Keep the previous bounded-shutdown protection against native/TLS thread
    // teardown that can outlive the C++ worker function on Windows.
    worker_.detach();

    qInfo() << "[shutdown] shutdownForExit end";
    return true;
}

QStringList RuntimeController::cameraNames() const
{
    QStringList names;
    names.reserve(cameras_.size());

    for (const auto &camera : cameras_) {
        names.append(camera.description());
    }

    return names;
}

QStringList RuntimeController::formatNames() const
{
    QStringList names;
    names.reserve(formats_.size());

    for (const auto &format : formats_) {
        const auto size = format.resolution();

        const QString pixelFormat =
            QVideoFrameFormat::pixelFormatToString(
                format.pixelFormat());

        names.append(
            QStringLiteral(
                "%1 x %2 @ %3 FPS | %4")
                .arg(size.width())
                .arg(size.height())
                .arg(qRound(format.maxFrameRate()))
                .arg(pixelFormat));
    }

    return names;
}

QStringList RuntimeController::logicalKeys() const
{
    QStringList keys;

    const auto appendIfPressed = [this, &keys](KeyCode key, const char *name) {
        if ((controllerState_.keys & keyMask(key)) != 0U) {
            keys.append(QString::fromLatin1(name));
        }
    };

    appendIfPressed(KeyCode::Space, "Space");
    appendIfPressed(KeyCode::Enter, "Enter");
    appendIfPressed(KeyCode::Escape, "Escape");
    appendIfPressed(KeyCode::Tab, "Tab");
    appendIfPressed(KeyCode::Left, "Left");
    appendIfPressed(KeyCode::Right, "Right");
    appendIfPressed(KeyCode::Up, "Up");
    appendIfPressed(KeyCode::Down, "Down");
    appendIfPressed(KeyCode::Ctrl, "Ctrl");
    appendIfPressed(KeyCode::Shift, "Shift");
    appendIfPressed(KeyCode::Alt, "Alt");

    return keys;
}

QVariantList RuntimeController::mappingRows() const
{
    QVariantList rows;
    rows.reserve(static_cast<qsizetype>(activeMappingProfile_.mappings.size()));

    for (const auto &rule : activeMappingProfile_.mappings) {
        QVariantMap row;
        row.insert(QStringLiteral("id"), QString::fromStdString(rule.id));
        row.insert(QStringLiteral("hand"), rule.hand == HandSide::Left
            ? QStringLiteral("Left")
            : QStringLiteral("Right"));
        row.insert(QStringLiteral("source"), gestureDisplayName(rule.gesture));
        row.insert(QStringLiteral("type"), QStringLiteral("Gesture"));
        row.insert(QStringLiteral("action"), actionDisplayName(rule.action));
        row.insert(QStringLiteral("output"), actionOutputName(rule.action));
        row.insert(QStringLiteral("behavior"), QString::fromLatin1(actionBehaviorName(rule.behavior)));
        row.insert(QStringLiteral("state"), rule.enabled
            ? QStringLiteral("Enabled")
            : QStringLiteral("Disabled"));
        row.insert(QStringLiteral("enabled"), rule.enabled);
        rows.append(row);
    }

    return rows;
}

QString RuntimeController::mappingProfileDirectory() const
{
    return profileStore_
        ? profileStore_->writableDirectory()
        : QString{};
}

QVariantList RuntimeController::mappingIssues() const
{
    QVariantList result;
    for (const auto &issue : validateMappings(activeMappingProfile_)) {
        QVariantMap item;
        item.insert(QStringLiteral("code"), QString::fromStdString(issue.code));
        item.insert(QStringLiteral("severity"), issue.severity == MappingIssue::Severity::Error
                        ? QStringLiteral("error") : QStringLiteral("warning"));
        item.insert(QStringLiteral("first"), issue.first);
        item.insert(QStringLiteral("second"), issue.second);
        result.append(item);
    }
    return result;
}

bool RuntimeController::canDeleteActiveProfile() const
{
    // Bundled profiles are seeded again at startup; don't offer delete for them.
    static const QStringList bundled{QStringLiteral("Default"), QStringLiteral("Desktop"),
                                     QStringLiteral("Game"), QStringLiteral("Presentation")};
    return !bundled.contains(activeProfile_, Qt::CaseInsensitive);
}

void RuntimeController::initializeProfiles()
{
    QString bundledDirectory =
        QDir(QCoreApplication::applicationDirPath())
            .filePath(QStringLiteral("profiles"));

    if (!QDir(bundledDirectory).exists()) {
        bundledDirectory = QDir(QStringLiteral(VC_SOURCE_ROOT))
            .filePath(QStringLiteral("profiles"));
    }

    QString writableRoot =
        QStandardPaths::writableLocation(
            QStandardPaths::AppConfigLocation);

    if (writableRoot.isEmpty()) {
        writableRoot = QCoreApplication::applicationDirPath();
    }

    const QString writableDirectory =
        QDir(writableRoot).filePath(QStringLiteral("profiles"));

    profileStore_ = std::make_unique<ProfileStore>(
        bundledDirectory,
        writableDirectory);

    QString error;
    if (!profileStore_->initialize(&error)) {
        mappingError_ = error;
        return;
    }

    mappingProfileNames_ = profileStore_->profileNames();
    if (mappingProfileNames_.removeAll(QStringLiteral("Default")) > 0) {
        mappingProfileNames_.prepend(QStringLiteral("Default"));
    }

    if (!loadProfile(QStringLiteral("Default"))
        && !mappingProfileNames_.isEmpty()) {
        loadProfile(mappingProfileNames_.constFirst());
    }
}

bool RuntimeController::loadProfile(const QString &name)
{
    if (!profileStore_) {
        mappingError_ = QStringLiteral("Profile store is unavailable.");
        emit mappingChanged();
        return false;
    }

    MappingProfile profile;
    QString error;
    if (!profileStore_->load(name, &profile, &error)) {
        mappingError_ = error;
        emit mappingChanged();
        return false;
    }

    if (outputService_.status().requested) {
        outputService_.pause("Profile or gesture mapping changed");
        refreshOutputStatus();
    }

    activeMappingProfile_ = std::move(profile);
    activeProfile_ = name;
    mappingError_.clear();
    applyActiveMappings();
    gestureStateManager_.reset();
    leftGestureEvent_ = QStringLiteral("IDLE");
    rightGestureEvent_ = QStringLiteral("IDLE");
    emit mappingChanged();
    emit stateChanged();
    return true;
}

void RuntimeController::applyActiveMappings()
{
    actionMapper_.setMappings(activeMappingProfile_.mappings);
}

void RuntimeController::setActiveProfile(const QString &name)
{
    if (name.isEmpty() || name == activeProfile_) {
        return;
    }
    loadProfile(name);
}

int RuntimeController::addMapping()
{
    // Mapping edits are a safety boundary. Disarm before changing the
    // ActionMapper so no held/toggled OS input survives an editor operation.
    if (outputService_.status().requested) {
        outputService_.pause("Profile or gesture mapping changed");
        refreshOutputStatus();
    }

    int suffix = 1;
    std::string id;
    for (;;) {
        id = "custom_" + std::to_string(suffix++);
        const bool exists = std::any_of(
            activeMappingProfile_.mappings.begin(),
            activeMappingProfile_.mappings.end(),
            [&id](const MappingRule &rule) { return rule.id == id; });
        if (!exists) break;
    }

    MappingRule rule;
    rule.id = std::move(id);
    rule.hand = HandSide::Right;
    rule.gesture = GestureClass::Pinch;
    rule.action = LogicalAction::MouseLeft;
    rule.behavior = ActionBehavior::Hold;
    rule.enabled = false;
    activeMappingProfile_.mappings.push_back(rule);
    applyActiveMappings();
    mappingError_.clear();
    emit mappingChanged();
    emit stateChanged();
    return static_cast<int>(activeMappingProfile_.mappings.size()) - 1;
}

void RuntimeController::removeMapping(int index)
{
    if (index < 0
        || index >= static_cast<int>(activeMappingProfile_.mappings.size())) {
        return;
    }

    if (outputService_.status().requested) {
        outputService_.pause("Profile or gesture mapping changed");
        refreshOutputStatus();
    }

    activeMappingProfile_.mappings.erase(
        activeMappingProfile_.mappings.begin() + index);
    applyActiveMappings();
    saveActiveProfile();
    emit stateChanged();
}

bool RuntimeController::updateMapping(
    int index,
    const QString &hand,
    const QString &gesture,
    const QString &action,
    const QString &behavior,
    bool enabled)
{
    if (index < 0
        || index >= static_cast<int>(activeMappingProfile_.mappings.size())) {
        mappingError_ = QStringLiteral("Invalid mapping row.");
        emit mappingChanged();
        return false;
    }

    const auto parsedHand = parseHandName(hand);
    const auto parsedGesture = parseGestureName(gesture);
    const auto parsedAction = parseActionName(action);
    const auto parsedBehavior = parseBehaviorName(behavior);

    if (!parsedHand || !parsedGesture || !parsedAction || !parsedBehavior
        || *parsedGesture == GestureClass::None) {
        mappingError_ = QStringLiteral("Invalid mapping values.");
        emit mappingChanged();
        return false;
    }

    if (*parsedBehavior == ActionBehavior::Toggle
        && (*parsedAction == LogicalAction::ScrollUp
            || *parsedAction == LogicalAction::ScrollDown)) {
        mappingError_ = QStringLiteral(
            "Toggle is not allowed for mouse wheel actions. Use Press or Hold.");
        emit mappingChanged();
        return false;
    }

    if (*parsedAction == LogicalAction::CursorMoveEnable
        && *parsedBehavior == ActionBehavior::Press) {
        mappingError_ = QStringLiteral(
            "Enable cursor movement supports Hold or Toggle. "
            "Use Hold to move only while the gesture is held, or Toggle to arm/disarm movement.");
        emit mappingChanged();
        return false;
    }

    if (outputService_.status().requested) {
        outputService_.pause("Profile or gesture mapping changed");
        refreshOutputStatus();
    }

    auto &rule = activeMappingProfile_.mappings[static_cast<std::size_t>(index)];
    rule.hand = *parsedHand;
    rule.gesture = *parsedGesture;
    rule.action = *parsedAction;
    rule.behavior = *parsedBehavior;
    rule.enabled = enabled;

    applyActiveMappings();
    const bool saved = saveActiveProfile();
    gestureStateManager_.reset();
    leftGestureEvent_ = QStringLiteral("IDLE");
    rightGestureEvent_ = QStringLiteral("IDLE");
    emit stateChanged();
    return saved;
}

bool RuntimeController::saveActiveProfile()
{
    if (!profileStore_) {
        mappingError_ = QStringLiteral("Profile store is unavailable.");
        emit mappingChanged();
        return false;
    }

    activeMappingProfile_.name = activeProfile_.toStdString();
    QString error;
    const bool ok = profileStore_->save(activeMappingProfile_, &error);
    mappingError_ = ok ? QString{} : error;

    mappingProfileNames_ = profileStore_->profileNames();
    if (mappingProfileNames_.removeAll(QStringLiteral("Default")) > 0) {
        mappingProfileNames_.prepend(QStringLiteral("Default"));
    }

    emit mappingChanged();
    return ok;
}

bool RuntimeController::reloadActiveProfile()
{
    return loadProfile(activeProfile_);
}

bool RuntimeController::duplicateActiveProfile(const QString &newName)
{
    const QString name = newName.trimmed();
    if (!profileStore_ || !ProfileStore::validName(name)) {
        mappingError_ = QStringLiteral("Invalid new profile name. Use 1-64 filename-safe characters.");
        emit mappingChanged();
        return false;
    }
    for (const auto &existing : mappingProfileNames_) {
        if (existing.compare(name, Qt::CaseInsensitive) == 0) {
            mappingError_ = QStringLiteral("A profile with that name already exists.");
            emit mappingChanged();
            return false;
        }
    }
    MappingProfile copy = activeMappingProfile_;
    copy.name = name.toStdString();
    QString error;
    if (!profileStore_->save(copy, &error)) {
        mappingError_ = error;
        emit mappingChanged();
        return false;
    }
    // Selecting another profile automatically disarms output and resets held keys.
    mappingProfileNames_ = profileStore_->profileNames();
    if (mappingProfileNames_.removeAll(QStringLiteral("Default")) > 0)
        mappingProfileNames_.prepend(QStringLiteral("Default"));
    return loadProfile(name);
}

bool RuntimeController::deleteProfile(const QString &name)
{
    const QString normalized = name.trimmed();
    static const QStringList bundled{QStringLiteral("Default"), QStringLiteral("Desktop"),
                                     QStringLiteral("Game"), QStringLiteral("Presentation")};
    if (!profileStore_ || bundled.contains(normalized, Qt::CaseInsensitive)
        || !ProfileStore::validName(normalized)) {
        mappingError_ = QStringLiteral("Built-in or invalid profiles cannot be deleted.");
        emit mappingChanged();
        return false;
    }
    if (!mappingProfileNames_.contains(normalized, Qt::CaseInsensitive)) {
        mappingError_ = QStringLiteral("Profile does not exist.");
        emit mappingChanged();
        return false;
    }
    // Always switch away before deleting active data. loadProfile disarms input.
    if (activeProfile_ == normalized && !loadProfile(QStringLiteral("Default")))
        return false;
    QString error;
    if (!profileStore_->remove(normalized, &error)) {
        mappingError_ = error;
        emit mappingChanged();
        return false;
    }
    mappingProfileNames_ = profileStore_->profileNames();
    if (mappingProfileNames_.removeAll(QStringLiteral("Default")) > 0)
        mappingProfileNames_.prepend(QStringLiteral("Default"));
    mappingError_.clear();
    emit mappingChanged();
    return true;
}

QString RuntimeController::runtimeStateName() const
{
    switch (runtimeState_) {
    case RuntimeState::Stopped:
        return QStringLiteral("Stopped");
    case RuntimeState::Starting:
        return QStringLiteral("Starting");
    case RuntimeState::Running:
        return QStringLiteral("Running");
    case RuntimeState::Stopping:
        return QStringLiteral("Stopping");
    case RuntimeState::Faulted:
        return QStringLiteral("Faulted");
    }

    return QStringLiteral("Unknown");
}

QString RuntimeController::trackingStatus() const
{
    if (runtimeState_ == RuntimeState::Faulted) {
        return QStringLiteral("Error");
    }

    if (runtimeState_ == RuntimeState::Stopped) {
        return QStringLiteral("Stopped");
    }

    if (runtimeState_ == RuntimeState::Stopping) {
        return QStringLiteral("Stopping");
    }

    if (runtimeState_ == RuntimeState::Starting
        || !trackerReady_) {
        return QStringLiteral("Starting");
    }

    const auto lastFrameUs =
        metricsSnapshot_.lastFrameReceivedUs;

    if (lastFrameUs <= 0) {
        return QStringLiteral("Waiting for frames");
    }

    if (nowUs() - lastFrameUs > 500000) {
        return QStringLiteral("No frames");
    }

    if (leftTracked_ && rightTracked_) {
        return QStringLiteral("Stable");
    }

    if (leftTracked_ || rightTracked_) {
        return QStringLiteral("Degraded");
    }

    return QStringLiteral("No hands");
}

void RuntimeController::setRuntimeState(
    RuntimeState state)
{
    if (runtimeState_ == state) {
        return;
    }

    qInfo()
        << "[runtime] state"
        << runtimeStateName()
        << "->";

    runtimeState_ = state;

    qInfo()
        << "[runtime] state now"
        << runtimeStateName();

    emit stateChanged();
}

void RuntimeController::failWithoutSession(
    const QString &message)
{
    errorMessage_ = message;
    trackerReady_ = false;
    cameraRunning_ = false;
    outputService_.pause("Pipeline fault - waiting for recovery");
    refreshOutputStatus();
    clearTracking();
    metricsSnapshot_ = {};

    setRuntimeState(
        RuntimeState::Faulted);

    emit stateChanged();
    maybeEmitApplicationExitReady();
}

void RuntimeController::setDetectionConfidence(
    int value)
{
    value = std::clamp(value, 0, 100);

    if (detectionConfidence_ == value) {
        return;
    }

    detectionConfidence_ = value;
    emit settingsChanged();
}

void RuntimeController::setTrackingConfidence(
    int value)
{
    value = std::clamp(value, 0, 100);

    if (trackingConfidence_ == value) {
        return;
    }

    trackingConfidence_ = value;
    emit settingsChanged();
}

void RuntimeController::setSwapHandedness(
    bool value)
{
    if (swapHandedness_.exchange(value) == value) {
        return;
    }

    emit settingsChanged();
}

void RuntimeController::setOutputArmed(bool value)
{
    refreshOutputStatus();
    if (outputSnapshot_.requested == value)
        return;

    if (value) {
        if (hasBlockingMappingIssues(activeMappingProfile_)) {
            mappingError_ = QStringLiteral(
                "Profile contains invalid mappings. Fix errors before enabling system input.");
            emit mappingChanged();
            emit stateChanged();
            return;
        }
        // These modes must *not* silently resume system input later. The
        // user explicitly enables it again after leaving them.
        if (playgroundMode_ || recorder_.active()
            || calibrator_.capturing() || rangeCalibrator_.capturing()
            || runtimeState_ != RuntimeState::Running
            || !outputSnapshot_.ready) {
            emit stateChanged();
            return;
        }
        resetContinuousControl();
        actionMapper_.reset();
        outputService_.arm(); // Persistent request; waits for stable frames.
    }
    else {
        outputService_.stop(); // Explicit user OFF cancels every auto-resume.
        actionMapper_.reset();
        if (cameraReconnectPolicy_.pending() && runtimeState_ != RuntimeState::Running)
            cancelCameraRecovery();
    }
    refreshOutputStatus();
    emit stateChanged();
}

void RuntimeController::setSensitivity(
    int value)
{
    value = std::clamp(value, 0, 100);

    if (sensitivity_ == value) {
        return;
    }

    sensitivity_ = value;
    emit settingsChanged();
}

void RuntimeController::setSmoothing(
    bool value)
{
    if (smoothing_ == value) {
        return;
    }

    smoothing_ = value;
    resetContinuousControl();
    emit settingsChanged();
}

void RuntimeController::setCursorSpeedPercent(
    int value)
{
    value = std::clamp(value, 50, 400);

    if (cursorSpeedPercent_ == value) {
        return;
    }

    cursorSpeedPercent_ = value;
    emit settingsChanged();
}

void RuntimeController::setInvertX(
    bool value)
{
    if (invertX_ == value) {
        return;
    }

    invertX_ = value;
    emit settingsChanged();
}

void RuntimeController::setInvertY(
    bool value)
{
    if (invertY_ == value) {
        return;
    }

    invertY_ = value;
    emit settingsChanged();
}

void RuntimeController::setDeadzone(
    int value)
{
    value = std::clamp(value, 0, 100);

    if (deadzone_ == value) {
        return;
    }

    deadzone_ = value;
    emit settingsChanged();
}

void RuntimeController::setRecognitionThreshold(
    int value)
{
    value = std::clamp(value, 0, 100);

    if (recognitionThreshold_.exchange(value) == value) {
        return;
    }

    emit settingsChanged();
}

QVariantMap RuntimeController::gestureThresholdOverrides() const
{
    QVariantMap values;
    for (std::size_t idx = 1; idx < kGestureClassCount; ++idx) {
        values.insert(QString::fromLatin1(kGestureCanonicalNames[idx]),
                      gestureThresholdOverrides_[idx].load());
    }
    return values;
}

QVariantMap RuntimeController::gestureThresholds() const
{
    QVariantMap values;
    const int global = recognitionThreshold_.load();
    for (std::size_t idx = 1; idx < kGestureClassCount; ++idx) {
        const int custom = gestureThresholdOverrides_[idx].load();
        values.insert(QString::fromLatin1(kGestureCanonicalNames[idx]),
                      custom < 0 ? global : custom);
    }
    return values;
}

void RuntimeController::setGestureThreshold(const QString &name, int value)
{
    const auto gesture = parseGestureName(name);
    if (!gesture || *gesture == GestureClass::None)
        return;
    // -1 means inherit global; any explicit override has a conservative floor.
    value = value < 0 ? -1 : std::clamp(value, 60, 100);
    if (gestureThresholdOverrides_[gestureIndex(*gesture)].exchange(value) != value)
        emit settingsChanged();
}

void RuntimeController::resetGestureThresholds()
{
    bool changed = false;
    for (std::size_t idx = 1; idx < kGestureClassCount; ++idx)
        changed |= gestureThresholdOverrides_[idx].exchange(-1) != -1;
    if (changed) emit settingsChanged();
}

QString RuntimeController::calibrationStatus() const
{
    switch (calibrator_.status()) {
    case GestureCalibrator::Status::Capturing: return QStringLiteral("CAPTURING");
    case GestureCalibrator::Status::Ready: return QStringLiteral("READY");
    case GestureCalibrator::Status::WeakSignal: return QStringLiteral("WEAK_SIGNAL");
    case GestureCalibrator::Status::Ambiguous: return QStringLiteral("AMBIGUOUS");
    case GestureCalibrator::Status::Cancelled: return QStringLiteral("CANCELLED");
    default: return QStringLiteral("IDLE");
    }
}

QString RuntimeController::calibrationGesture() const
{
    return QString::fromLatin1(gestureCanonicalName(calibrator_.selectedGesture()));
}

QString RuntimeController::calibrationHand() const
{
    return calibrator_.selectedSide() == HandSide::Left
        ? QStringLiteral("Left") : QStringLiteral("Right");
}

QVariantList RuntimeController::calibrationResults() const
{
    QVariantList results;
    for (std::size_t side = 0; side < kHandCount; ++side) {
        for (std::size_t idx = 1; idx < kGestureClassCount; ++idx) {
            const auto gesture = static_cast<GestureClass>(idx);
            const auto r = calibrator_.result(static_cast<HandSide>(side), gesture);
            QVariantMap item;
            item.insert(QStringLiteral("hand"), side == 0 ? QStringLiteral("Left") : QStringLiteral("Right"));
            item.insert(QStringLiteral("gesture"), QString::fromLatin1(kGestureCanonicalNames[idx]));
            item.insert(QStringLiteral("valid"), r.valid);
            item.insert(QStringLiteral("threshold"), r.threshold);
            item.insert(QStringLiteral("samples"), r.samples);
            item.insert(QStringLiteral("dominance"), r.dominantPercent);
            item.insert(QStringLiteral("suggested"), calibrator_.suggestedThreshold(gesture));
            results.append(item);
        }
    }
    return results;
}

bool RuntimeController::startGestureCalibration(const QString &hand, const QString &name)
{
    const auto gesture = parseGestureName(name);
    if (!gesture || *gesture == GestureClass::None || runtimeState_ != RuntimeState::Running)
        return false;
    const HandSide side = hand.compare(QStringLiteral("Left"), Qt::CaseInsensitive) == 0
        ? HandSide::Left : HandSide::Right;
    if (hand.compare(QStringLiteral("Left"), Qt::CaseInsensitive) != 0
        && hand.compare(QStringLiteral("Right"), Qt::CaseInsensitive) != 0)
        return false;
    stopCursorPlayground();
    cancelRangeCalibration();
    setOutputArmed(false);
    calibrator_.start(side, *gesture);
    emit stateChanged();
    return true;
}

void RuntimeController::cancelGestureCalibration()
{
    calibrator_.cancel();
    emit stateChanged();
}

bool RuntimeController::applyGestureCalibration(const QString &gesture)
{
    const auto klass = parseGestureName(gesture);
    if (!klass || *klass == GestureClass::None || calibrator_.capturing())
        return false;
    const int recommended = calibrator_.suggestedThreshold(*klass);
    if (recommended < 0) return false;
    setGestureThreshold(gesture, recommended);
    return true;
}

void RuntimeController::clearGestureCalibration()
{
    calibrator_.clearResults();
    emit stateChanged();
}

QVariantMap RuntimeController::cursorPlaygroundState() const
{
    QVariantMap state;
    const auto point = cursorPlayground_.cursor();
    const auto goal = cursorPlayground_.target();
    state.insert(QStringLiteral("active"), cursorPlayground_.active());
    state.insert(QStringLiteral("complete"), cursorPlayground_.complete());
    state.insert(QStringLiteral("tracked"), cursorPlayground_.tracked());
    state.insert(QStringLiteral("x"), point.x);
    state.insert(QStringLiteral("y"), point.y);
    state.insert(QStringLiteral("targetX"), goal.x);
    state.insert(QStringLiteral("targetY"), goal.y);
    state.insert(QStringLiteral("targetIndex"), cursorPlayground_.targetIndex());
    state.insert(QStringLiteral("targetCount"), cursorPlayground_.totalTargets());
    state.insert(QStringLiteral("dwellPercent"), cursorPlayground_.dwellPercent(playgroundSampleUs_));
    state.insert(QStringLiteral("averageMs"), cursorPlayground_.averageAcquisitionMs());
    state.insert(QStringLiteral("averageErrorPx"), cursorPlayground_.averageErrorPx());
    state.insert(QStringLiteral("jitterPx"), cursorPlayground_.averageJitterPx());
    state.insert(QStringLiteral("missedClicks"), cursorPlayground_.missedClicks());
    return state;
}

bool RuntimeController::startCursorPlayground()
{
    if (runtimeState_ != RuntimeState::Running)
        return false;
    cancelGestureCalibration();
    cancelRangeCalibration();
    setOutputArmed(false);
    resetContinuousControl();
    playgroundSampleUs_ = nowUs();
    cursorPlayground_.start(playgroundSampleUs_);
    playgroundMode_ = true;
    emit stateChanged();
    return true;
}

void RuntimeController::stopCursorPlayground()
{
    if (playgroundMode_) {
        playgroundMode_ = false;
        cursorPlayground_.stop();
        emit stateChanged();
    }
}

void RuntimeController::setDebounceMs(int value)
{
    value = std::clamp(value, 0, 1000);
    if (debounceMs_ == value) return;
    debounceMs_ = value;
    resetGestureActions();
    emit settingsChanged();
}

void RuntimeController::setCooldownMs(int value)
{
    value = std::clamp(value, 0, 2000);
    if (cooldownMs_ == value) return;
    cooldownMs_ = value;
    resetGestureActions();
    emit settingsChanged();
}

void RuntimeController::setRequireRelease(bool value)
{
    if (requireRelease_ == value) return;
    requireRelease_ = value;
    resetGestureActions();
    emit settingsChanged();
}

void RuntimeController::setGestureGraceMs(int value)
{
    value = std::clamp(value, 0, 500);
    if (gestureGraceMs_ == value) return;
    gestureGraceMs_ = value;
    resetGestureActions();
    emit settingsChanged();
}

void RuntimeController::setGestureReleaseThreshold(int value)
{
    value = std::clamp(value, 0, 100);
    if (gestureReleaseThreshold_ == value) return;
    gestureReleaseThreshold_ = value;
    resetGestureActions();
    emit settingsChanged();
}

QString RuntimeController::recordingsDirectory() const
{
    const QString custom = QSettings().value(
        QStringLiteral("recording/directory")).toString();
    if (!custom.isEmpty())
        return QDir::cleanPath(custom);
    return QDir::cleanPath(QStandardPaths::writableLocation(
        QStandardPaths::AppLocalDataLocation) + QStringLiteral("/gesture_sessions"));
}

QVariantMap RuntimeController::recordingQuality() const
{
    const auto metrics = [this](HandSide side) {
        QVariantMap item;
        const auto valid = recorder_.usableFrames(side);
        const auto lost = recorder_.missingFrames(side);
        item.insert(QStringLiteral("labelled"), recorder_.labelled(side));
        item.insert(QStringLiteral("label"), recorder_.gestureLabel(side)
            ? QString::fromLatin1(gestureCanonicalName(*recorder_.gestureLabel(side)))
            : QStringLiteral("UNLABELED"));
        item.insert(QStringLiteral("usable"), static_cast<int>(valid));
        item.insert(QStringLiteral("missing"), static_cast<int>(lost));
        item.insert(QStringLiteral("percent"), static_cast<int>(
            std::min<std::size_t>(100, 100 * valid /
                GestureDataRecorder::kSuggestedFramesPerHand)));
        item.insert(QStringLiteral("trackedPercent"),
            valid + lost ? static_cast<int>(100 * valid / (valid + lost)) : 0);
        return item;
    };
    return {{QStringLiteral("target"),
             static_cast<int>(GestureDataRecorder::kSuggestedFramesPerHand)},
            {QStringLiteral("left"), metrics(HandSide::Left)},
            {QStringLiteral("right"), metrics(HandSide::Right)}};
}

void RuntimeController::setRecordingsError(const QString &message)
{
    recordingsError_ = message;
    emit recordingsChanged();
}

bool RuntimeController::setRecordingsDirectory(const QUrl &folder)
{
    if (recorder_.active()) {
        setRecordingsError(QStringLiteral("Stop recording before changing the folder."));
        return false;
    }
    if (!folder.isLocalFile() || folder.toLocalFile().isEmpty()) {
        setRecordingsError(QStringLiteral("Choose a local folder."));
        return false;
    }
    QDir dir(QDir::cleanPath(folder.toLocalFile()));
    if (!dir.isAbsolute() || !dir.exists()
        || !QFileInfo(dir.absolutePath()).isWritable()) {
        setRecordingsError(QStringLiteral("Selected folder is not writable."));
        return false;
    }
    QSettings().setValue(QStringLiteral("recording/directory"), dir.absolutePath());
    setRecordingsError(QString{});
    return true;
}

void RuntimeController::resetRecordingsDirectory()
{
    if (recorder_.active()) {
        setRecordingsError(QStringLiteral("Stop recording before changing the folder."));
        return;
    }
    QSettings().remove(QStringLiteral("recording/directory"));
    setRecordingsError(QString{});
}

QVariantList RuntimeController::recordings() const
{
    QVariantList entries;
    QDir dir(recordingsDirectory());
    if (!dir.exists())
        return entries;
    const auto files = dir.entryInfoList(
        QStringList{QStringLiteral("gesture_session_*.jsonl")},
        QDir::Files | QDir::Readable | QDir::NoSymLinks,
        QDir::Time);
    for (const auto &file : files) {
        if (entries.size() >= 100)
            break;
        QVariantMap entry;
        entry.insert(QStringLiteral("name"), file.fileName());
        entry.insert(QStringLiteral("path"), file.absoluteFilePath());
        entry.insert(QStringLiteral("size"), file.size());
        entry.insert(QStringLiteral("modified"),
                     file.lastModified().toString(QStringLiteral("yyyy-MM-dd HH:mm")));
        entry.insert(QStringLiteral("active"), recorder_.active()
            && QDir::cleanPath(file.absoluteFilePath()) == QDir::cleanPath(recordingPath_));
        entries.append(entry);
    }
    return entries;
}

QString RuntimeController::validatedRecordingPath(const QString &name) const
{
    // Only the session filenames produced by this application may be managed.
    // A caller cannot escape the selected directory via '../' or symlinks.
    if (!name.startsWith(QStringLiteral("gesture_session_"))
        || !name.endsWith(QStringLiteral(".jsonl"))
        || name != QFileInfo(name).fileName()
        || name.contains(QLatin1Char('/'))
        || name.contains(QLatin1Char('\\')))
        return {};
    const QFileInfo info(QDir(recordingsDirectory()).filePath(name));
    if (!info.isFile() || info.isSymLink())
        return {};
    return info.absoluteFilePath();
}

void RuntimeController::openRecordingsFolder()
{
    QDir dir(recordingsDirectory());
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        setRecordingsError(QStringLiteral("Cannot create the recordings folder."));
        return;
    }
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(dir.absolutePath()))) {
        setRecordingsError(QStringLiteral("Could not open the recordings folder."));
        return;
    }
    setRecordingsError(QString{});
}

bool RuntimeController::openRecordingFile(const QString &name)
{
    const QString path = validatedRecordingPath(name);
    if (path.isEmpty() || !QDesktopServices::openUrl(QUrl::fromLocalFile(path))) {
        setRecordingsError(QStringLiteral("Cannot open the selected recording."));
        return false;
    }
    setRecordingsError(QString{});
    return true;
}

bool RuntimeController::deleteRecordingFile(const QString &name)
{
    if (recorder_.active() || replayTimer_.isActive()) {
        setRecordingsError(QStringLiteral("Stop recording or replay before deleting files."));
        return false;
    }
    const QString path = validatedRecordingPath(name);
    if (path.isEmpty()) {
        setRecordingsError(QStringLiteral("Recording not found."));
        return false;
    }
    QFile file(path);
    if (!file.moveToTrash()) {
        setRecordingsError(QStringLiteral("Cannot move the recording to Recycle Bin: %1")
                           .arg(file.errorString()));
        return false;
    }
    if (QDir::cleanPath(recordingPath_) == QDir::cleanPath(path))
        recordingPath_.clear();
    setRecordingsError(QString{});
    emit stateChanged();
    return true;
}

bool RuntimeController::replayRecordingFile(const QString &name)
{
    const QString path = validatedRecordingPath(name);
    if (path.isEmpty()) {
        setRecordingsError(QStringLiteral("Recording not found."));
        return false;
    }
    setRecordingsError(QString{});
    return startGestureReplay(path);
}

bool RuntimeController::startGestureRecording(
    const QString &left, const QString &right)
{
    if (runtimeState_ != RuntimeState::Running || recorder_.active()) {
        recordingError_ = QStringLiteral("Start pipeline first; only one recording at a time");
        emit stateChanged();
        return false;
    }
    GestureDataRecorder::Labels labels{};
    for (const auto &selection : {std::pair{left, 0U}, std::pair{right, 1U}}) {
        if (selection.first != QStringLiteral("UNLABELED")) {
            auto label = parseGestureName(selection.first);
            if (!label) {
                recordingError_ = QStringLiteral("Invalid gesture label");
                emit stateChanged();
                return false;
            }
            labels[selection.second] = *label;
        }
    }

    if (!labels[0] && !labels[1]) {
        recordingError_ = QStringLiteral("Select at least one labelled hand.");
        emit stateChanged();
        return false;
    }
    // Dataset capture never generates system input, even with an armed profile.
    outputService_.stop();
    refreshOutputStatus();
    resetGestureActions();
    const QString directory = recordingsDirectory();
    const QByteArray utf8 = directory.toUtf8();
    if (!recorder_.start(std::filesystem::u8path(utf8.constData()), labels)) {
        recordingError_ = QString::fromStdString(recorder_.error());
        emit stateChanged();
        return false;
    }
    recordingPath_ = filePathDisplay(recorder_.path());
    recordingError_.clear();
    lastRecordingQualitySignalUs_ = 0;
    emit recordingQualityChanged();
    emit recordingsChanged();
    emit stateChanged();
    return true;
}

void RuntimeController::stopGestureRecording()
{
    if (!recorder_.active()) return;
    recorder_.stop();
    recordingError_ = QString::fromStdString(recorder_.error());
    lastRecordingQualitySignalUs_ = 0;
    emit recordingQualityChanged();
    emit recordingsChanged();
    emit stateChanged();
}

void RuntimeController::setAutoRecoverCamera(bool enabled)
{
    if (autoRecoverCamera_ == enabled)
        return;
    autoRecoverCamera_ = enabled;
    if (!enabled)
        cancelCameraRecovery();
    emit settingsChanged();
}

void RuntimeController::scheduleCameraRecovery(const QByteArray &deviceId)
{
    if (!autoRecoverCamera_ || deviceId.isEmpty() || exitRequested_)
        return;
    if (cameraReconnectPolicy_.pending())
        return;

    // While the user still wants Windows input, keep waiting for the original
    // camera instead of giving up after 30 seconds. Never silently switch to
    // a different device. Backoff prevents a faulty driver from rapid loops.
    const bool persistent = outputService_.status().requested;
    const int pollLimit = persistent ? std::numeric_limits<int>::max() : 30;
    if (!cameraReconnectPolicy_.begin(deviceId.toStdString(), pollLimit))
        return;
    recoveryAttempt_ = std::min(recoveryAttempt_ + 1, 5);
    const std::int64_t delayUs = std::min<std::int64_t>(
        10'000'000, 1'000'000LL << recoveryAttempt_);
    recoveryNotBeforeUs_ = nowUs() + delayUs;
    cameraRecoveryStatus_ = QStringLiteral("Waiting for camera recovery");
    cameraRecoveryTimer_.start();
    emit stateChanged();
}

void RuntimeController::cancelCameraRecovery()
{
    cameraRecoveryTimer_.stop();
    cameraReconnectPolicy_.cancel();
    recoveryNotBeforeUs_ = 0;
    if (!cameraRecoveryStatus_.isEmpty()) {
        cameraRecoveryStatus_.clear();
        emit stateChanged();
    }
}

void RuntimeController::pollCameraRecovery()
{
    if (!autoRecoverCamera_ || !cameraReconnectPolicy_.pending()) {
        cancelCameraRecovery();
        return;
    }
    if (exitRequested_) {
        cancelCameraRecovery();
        return;
    }
    if (nowUs() < recoveryNotBeforeUs_)
        return;

    const auto connected = QMediaDevices::videoInputs();
    const std::string wanted = cameraReconnectPolicy_.deviceId();
    auto found = std::find_if(connected.begin(), connected.end(),
        [&wanted](const QCameraDevice &device) {
            return device.id().toStdString() == wanted;
        });
    const bool stopped = canStart() && !pipelineRunning();
    const auto decision = cameraReconnectPolicy_.poll(found != connected.end(), stopped);
    if (decision == CameraReconnectPolicy::Decision::TimedOut) {
        cameraRecoveryTimer_.stop();
        cameraRecoveryStatus_ = QStringLiteral("Camera reconnection timed out");
        emit stateChanged();
        return;
    }
    if (decision != CameraReconnectPolicy::Decision::Restart)
        return;

    cameraRecoveryTimer_.stop();
    refreshDevices();
    const auto it = std::find_if(cameras_.begin(), cameras_.end(),
        [&wanted](const QCameraDevice &device) {
            return device.id().toStdString() == wanted;
        });
    if (it == cameras_.end()) {
        cameraRecoveryStatus_ = QStringLiteral("Camera unavailable");
        emit stateChanged();
        return;
    }
    cameraIndex_ = static_cast<int>(std::distance(cameras_.begin(), it));
    refreshFormats();
    emit devicesChanged();
    cameraRecoveryStatus_ = QStringLiteral("Camera reconnected - waiting for tracking");
    emit stateChanged();
    // start() preserves the enabled intent; output resumes only after fresh
    // tracked frames satisfy OutputResumePolicy.
    start();
}

void RuntimeController::refreshDevices()
{
    cameras_ = QMediaDevices::videoInputs();

    if (cameras_.isEmpty()) {
        cameraIndex_ = 0;
        formatIndex_ = 0;
        formats_.clear();
        resolution_ = QStringLiteral("-");

        emit devicesChanged();
        emit stateChanged();
        return;
    }

    cameraIndex_ =
        std::clamp(
            cameraIndex_,
            0,
            static_cast<int>(
                cameras_.size()) - 1);

    refreshFormats();
    emit devicesChanged();
}

void RuntimeController::refreshFormats()
{
    formats_.clear();
    formatIndex_ = 0;

    if (cameras_.isEmpty()) {
        resolution_ = QStringLiteral("-");
        return;
    }

    formats_ =
        cameras_[cameraIndex_]
            .videoFormats();

    std::sort(
        formats_.begin(),
        formats_.end(),
        [](const QCameraFormat &a,
           const QCameraFormat &b) {

            const auto score =
                [](const QCameraFormat &format) {
                    const auto size =
                        format.resolution();

                    const int resolutionPenalty =
                        std::abs(size.width() - 1280)
                        + std::abs(size.height() - 720);

                    const int fpsPenalty =
                        qRound(
                            std::abs(
                                format.maxFrameRate()
                                - 30.0F)
                            * 12.0F);

                    return resolutionPenalty
                        + fpsPenalty;
                };

            return score(a) < score(b);
        });

    if (!formats_.isEmpty()) {
        const auto size =
            formats_.front().resolution();

        resolution_ =
            QStringLiteral("%1 x %2")
                .arg(size.width())
                .arg(size.height());
    }
    else {
        resolution_ = QStringLiteral("Default");
    }
}

void RuntimeController::attachVideoOutput(QObject *sink)
{
    // Keep the public method name for existing QML integrations, but receive
    // VideoOutput.videoSink rather than the VideoOutput itself. Disconnecting
    // an inactive view must never stop the camera or the tracking worker.
    auto *preview = qobject_cast<QVideoSink *>(sink);
    if (sink && !preview) {
        qWarning() << "[controller preview] Expected a QVideoSink";
        return;
    }
    if (controllerSink_ == preview)
        return;
    controllerSink_ = preview;
    updatePreviewConnections();
}

void RuntimeController::attachPiPVideoSink(QObject *sink)
{
    auto *preview = qobject_cast<QVideoSink *>(sink);
    if (sink && !preview) {
        qWarning() << "[pip] Expected a QVideoSink";
        return;
    }
    if (pipSink_ == preview)
        return;
    pipSink_ = preview;
    updatePreviewConnections();
}

void RuntimeController::setPiPActive(bool active)
{
    if (pipActive_ == active)
        return;
    pipActive_ = active;
    updatePreviewConnections();
}

void RuntimeController::setMainWindowMinimized(bool minimized)
{
    if (mainWindowMinimized_ == minimized)
        return;

    mainWindowMinimized_ = minimized;
    // Rendering only: do not reset gestures, disarm output, restart camera,
    // or disturb an active JSONL recording. If capture briefly stalls while
    // minimized, OutputService temporarily releases held inputs and resumes
    // when fresh tracked frames return, without another ON click.
    updatePreviewConnections();
    qInfo() << "[window]" << (minimized ? "minimized" : "restored")
            << "output requested:" << outputService_.status().requested;
}

void RuntimeController::attachRecordingVideoSink(QObject *sink)
{
    auto *preview = qobject_cast<QVideoSink *>(sink);
    if (sink && !preview) {
        qWarning() << "[recording preview] Expected a QVideoSink";
        return;
    }
    if (recordingSink_ == preview)
        return;
    recordingSink_ = preview;
    updatePreviewConnections();
}

void RuntimeController::updatePreviewConnections()
{
    // The camera keeps capturing even with no visible preview. Pause only the
    // UI presentation route on navigation/minimization, never MediaPipe.
    previewCaptureEnabled_.store(false, std::memory_order_release);
    previewTimer_.stop();
    previewMailbox_.clear();
    // Qt Quick VideoOutput can otherwise retain the last QVideoFrame (and
    // possibly GPU/D3D11 resources) after its page becomes hidden. Release
    // all inactive preview references on the GUI thread on every route change.
    if (controllerSink_) controllerSink_->setVideoFrame(QVideoFrame{});
    if (recordingSink_) recordingSink_->setVideoFrame(QVideoFrame{});
    if (pipSink_) pipSink_->setVideoFrame(QVideoFrame{});
    ++previewGeneration_;
    activePreviewSink_ = nullptr;

    const auto target = selectPreviewTarget(
        bool(controllerSink_), bool(recordingSink_),
        bool(pipSink_), pipActive_, mainWindowMinimized_);
    int timerMs = 0;
    switch (target) {
    case PreviewTarget::Controller:
        activePreviewSink_ = controllerSink_;
        timerMs = 33;  // ~30 FPS; the full-rate worker is unaffected.
        break;
    case PreviewTarget::Recording:
        activePreviewSink_ = recordingSink_;
        timerMs = 50;  // ~20 FPS is sufficient for positioning gestures.
        break;
    case PreviewTarget::PiP:
        activePreviewSink_ = pipSink_;
        timerMs = 50;
        break;
    case PreviewTarget::None:
        return;
    }

    if (!activePreviewSink_ || activePreviewSink_ == &captureSink_)
        return;

    // This is the only place where the presentation sink is updated. Unlike
    // an AutoConnection slot attached to every incoming camera frame, the GUI
    // can never accumulate a backlog of obsolete preview delivery events.
    previewTimer_.start(timerMs);
    previewCaptureEnabled_.store(true, std::memory_order_release);
}

void RuntimeController::selectCamera(
    int index)
{
    cancelCameraRecovery();
    if (pipelineRunning()
        || index < 0
        || index >= static_cast<int>(
            cameras_.size())) {
        return;
    }

    if (cameraIndex_ == index) {
        return;
    }

    cameraIndex_ = index;
    refreshFormats();

    emit devicesChanged();
    emit stateChanged();
}

void RuntimeController::selectFormat(
    int index)
{
    cancelCameraRecovery();
    if (pipelineRunning()
        || index < 0
        || index >= static_cast<int>(
            formats_.size())) {
        return;
    }

    if (formatIndex_ == index) {
        return;
    }

    formatIndex_ = index;

    const auto size =
        formats_[formatIndex_]
            .resolution();

    resolution_ =
        QStringLiteral("%1 x %2")
            .arg(size.width())
            .arg(size.height());

    emit devicesChanged();
    emit stateChanged();
}

QString RuntimeController::mediaPipeLibraryPath() const
{
#ifdef _WIN32
    constexpr auto libraryName = "libmediapipe.dll";
#elif defined(__APPLE__)
    constexpr auto libraryName = "libmediapipe.dylib";
#else
    constexpr auto libraryName = "libmediapipe.so";
#endif

    const QString besideExecutable =
        QCoreApplication::applicationDirPath()
        + QStringLiteral("/native/")
        + QString::fromLatin1(libraryName);

    if (QFileInfo::exists(besideExecutable)) {
        return besideExecutable;
    }

    const QString configured =
        QString::fromUtf8(
            VC_MEDIAPIPE_ROOT_PATH)
        + QLatin1Char('/')
        + QString::fromLatin1(libraryName);

    if (QFileInfo::exists(configured)) {
        return configured;
    }

    return QString::fromUtf8(VC_SOURCE_ROOT)
        + QStringLiteral("/deps/mediapipe/")
        + QString::fromLatin1(libraryName);
}

QString RuntimeController::handLandmarkerModelPath() const
{
    const QString besideExecutable =
        QCoreApplication::applicationDirPath()
        + QStringLiteral(
            "/models/hand_landmarker.task");

    if (QFileInfo::exists(besideExecutable)) {
        return besideExecutable;
    }

    const QString configured =
        QString::fromUtf8(
            VC_HAND_LANDMARKER_MODEL_PATH);

    if (QFileInfo::exists(configured)) {
        return configured;
    }

    return QString::fromUtf8(VC_SOURCE_ROOT)
        + QStringLiteral(
            "/models/hand_landmarker.task");
}

QString RuntimeController::gestureModelPath() const
{
    const QString besideExecutable =
        QCoreApplication::applicationDirPath()
        + QStringLiteral(
            "/models/gesture_model.onnx");

    if (QFileInfo::exists(besideExecutable)) {
        return besideExecutable;
    }

#ifdef VC_GESTURE_MODEL_PATH
    const QString configured =
        QString::fromUtf8(
            VC_GESTURE_MODEL_PATH);

    if (QFileInfo::exists(configured)) {
        return configured;
    }
#endif

    return QString::fromUtf8(VC_SOURCE_ROOT)
        + QStringLiteral(
            "/models/gesture_model.onnx");
}

void RuntimeController::start()
{
    if (!canStart()) {
        return;
    }

    stopGestureReplay();
    // Explicit user start overrides a pending automatic reconnect request.
    // The policy has already disarmed itself before invoking start() when
    // called by pollCameraRecovery().
    if (cameraReconnectPolicy_.pending())
        cancelCameraRecovery();

    errorMessage_.clear();

    if (cameras_.isEmpty()) {
        failWithoutSession(
            QStringLiteral(
                "No camera detected. "
                "Check Windows camera permissions."));
        return;
    }

    const QString libraryPath =
        mediaPipeLibraryPath();

    const QString modelPath =
        handLandmarkerModelPath();

    if (!QFileInfo::exists(libraryPath)) {
        failWithoutSession(
            QStringLiteral(
                "Missing MediaPipe runtime: %1")
                .arg(libraryPath));
        return;
    }

    if (!QFileInfo::exists(modelPath)) {
        failWithoutSession(
            QStringLiteral(
                "Missing hand_landmarker.task: %1")
                .arg(modelPath));
        return;
    }

    auto *videoSink = &captureSink_;

    if (!videoSink) {
        failWithoutSession(
            QStringLiteral(
                "VideoOutput did not expose "
                "a QVideoSink."));
        return;
    }

    outputService_.pause("Waiting for new tracking session");
    refreshOutputStatus();
    clearTracking();
    resetContinuousControl();
    metricsSnapshot_ = {};
    captureStallHandled_ = false;
    sessionStartedUs_ = nowUs();
    lastTrackingUiNotifyUs_ = 0;
    lastDiagnosticsNotifyUs_ = 0;

    trackerReady_ = false;
    cameraRunning_ = false;
    terminalStateAfterStop_ =
        RuntimeState::Stopped;

    const std::uint64_t generation =
        ++generation_;

    inbox_ =
        std::make_shared<LatestFrameSlot>();

    metrics_ =
        std::make_shared<RuntimeMetrics>();

    std::weak_ptr<LatestFrameSlot> weakInbox =
        inbox_;

    std::weak_ptr<RuntimeMetrics> weakMetrics =
        metrics_;

    setRuntimeState(
        RuntimeState::Starting);

    frameConnection_ =
        connect(
            videoSink,
            &QVideoSink::videoFrameChanged,
            this,
            [weakInbox, weakMetrics](
                const QVideoFrame &frame) {

                if (!frame.isValid()) {
                    return;
                }

                const auto inbox =
                    weakInbox.lock();

                const auto metrics =
                    weakMetrics.lock();

                if (!inbox || !metrics
                    || inbox->isClosed()) {
                    return;
                }

                const auto captureUs = nowUs();

                const auto sequence =
                    metrics->recordFrameReceived(
                        captureUs);

                const auto publishResult =
                    inbox->publish({
                        frame,
                        captureUs,
                        sequence
                    });

                if (publishResult
                    == LatestFrameSlot::PublishResult::Replaced) {
                    metrics->recordFrameReplaced();
                }
            },
            Qt::DirectConnection);

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
            [this, generation](bool active) {
                if (generation != generation_) {
                    return;
                }

                cameraRunning_ = active;
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

                if (generation != generation_
                    || error == QCamera::NoError) {
                    return;
                }

                const QString resolved =
                    message.isEmpty()
                    ? QStringLiteral("Camera error")
                    : message;

                QMetaObject::invokeMethod(
                    this,
                    [this, generation, resolved] {
                        if (generation == generation_) {
                            errorMessage_ = resolved;
                            if (camera_ && autoRecoverCamera_)
                                scheduleCameraRecovery(camera_->cameraDevice().id());
                            requestSessionStop(RuntimeState::Faulted);
                        }
                    },
                    Qt::QueuedConnection);
            });

    WorkerSession session;
    session.inbox = inbox_;
    session.metrics = metrics_;
    session.libraryPath = libraryPath;
    session.modelPath = modelPath;
    session.gestureModelPath = gestureModelPath();
    session.detectionConfidence =
        static_cast<float>(
            detectionConfidence_)
        / 100.0F;
    session.trackingConfidence =
        static_cast<float>(
            trackingConfidence_)
        / 100.0F;
    session.generation = generation;

    {
        std::lock_guard lock(workerMutex_);

        // Lifecycle rules allow only one requested/active session. If this is
        // ever hit, drop the stale pending request rather than running it.
        if (pendingWorkerSession_) {
            if (pendingWorkerSession_->inbox) {
                pendingWorkerSession_
                    ->inbox
                    ->close();
            }

            pendingWorkerSession_.reset();
        }

        pendingWorkerSession_ =
            std::move(session);
    }

    workerCondition_.notify_one();

    camera_->start();
    statsTimer_.start();

    emit stateChanged();
}

void RuntimeController::teardownCameraAndFramePath(
    const std::shared_ptr<LatestFrameSlot> &stoppingInbox)
{
    disconnect(frameConnection_);

    if (stoppingInbox) {
        stoppingInbox->close();
    }

    disconnect(cameraActiveConnection_);
    disconnect(cameraErrorConnection_);

    if (camera_) {
        camera_->stop();
    }

    captureSession_.setCamera(nullptr);
    camera_.reset();
    cameraRunning_ = false;
}

void RuntimeController::requestSessionStop(
    RuntimeState terminalState)
{
    if (runtimeState_ == RuntimeState::Stopped) {
        if (terminalState == RuntimeState::Faulted) {
            setRuntimeState(RuntimeState::Faulted);
        }

        maybeEmitApplicationExitReady();
        return;
    }

    if (runtimeState_ == RuntimeState::Faulted) {
        maybeEmitApplicationExitReady();
        return;
    }

    if (runtimeState_ == RuntimeState::Stopping) {
        if (terminalState == RuntimeState::Faulted) {
            terminalStateAfterStop_ =
                RuntimeState::Faulted;
        }

        return;
    }

    terminalStateAfterStop_ = terminalState;
    trackerReady_ = false;

    // Neutralize physical inputs immediately. The explicit user STOP has
    // already cleared the request in stop(); camera faults/reloads must not.
    outputService_.pause("Camera or pipeline temporarily unavailable");
    refreshOutputStatus();
    resetContinuousControl();
    // Camera stop/fault/reconnect must close and flush any active dataset.
    // In particular never leave an asynchronous file writer running across
    // a newly started camera/tracking generation.
    if (recorder_.active())
        stopGestureRecording();

    setRuntimeState(
        RuntimeState::Stopping);

    const std::uint64_t stoppingGeneration =
        generation_;

    const auto stoppingInbox = inbox_;

    teardownCameraAndFramePath(
        stoppingInbox);

    bool workerOwnsSession = false;

    {
        std::lock_guard lock(workerMutex_);

        if (pendingWorkerSession_
            && pendingWorkerSession_->generation
                   == stoppingGeneration) {

            if (pendingWorkerSession_->inbox) {
                pendingWorkerSession_
                    ->inbox
                    ->close();
            }

            pendingWorkerSession_.reset();
        }

        workerOwnsSession =
            activeWorkerGeneration_
                .has_value()
            && *activeWorkerGeneration_
                   == stoppingGeneration;
    }

    workerCondition_.notify_all();

    inbox_.reset();
    statsTimer_.stop();
    clearTracking();

    emit stateChanged();

    if (!workerOwnsSession) {
        finalizeSessionStop(
            stoppingGeneration);
    }
}

void RuntimeController::finalizeSessionStop(
    std::uint64_t generation)
{
    if (generation != generation_
        || runtimeState_
               != RuntimeState::Stopping) {
        return;
    }

    trackerReady_ = false;
    cameraRunning_ = false;
    metrics_.reset();
    metricsSnapshot_ = {};
    clearTracking();

    const RuntimeState terminalState =
        terminalStateAfterStop_;

    if (terminalState == RuntimeState::Stopped) {
        errorMessage_.clear();
    }

    setRuntimeState(terminalState);
    emit stateChanged();

    // Reload starts only after teardown. Output remains physically neutral
    // until OutputResumePolicy sees a stable stream after restart.
    if (pendingModelReload_) {
        pendingModelReload_ = false;
        if (terminalState == RuntimeState::Stopped && !exitRequested_) {
            reloadQueued_ = true;
            QMetaObject::invokeMethod(this, [this] {
                if (!reloadQueued_ || exitRequested_ || runtimeState_ != RuntimeState::Stopped)
                    return;
                reloadQueued_ = false;
                start();
            }, Qt::QueuedConnection);
        }
    }
    maybeEmitApplicationExitReady();
}

void RuntimeController::handleWorkerFault(
    std::uint64_t generation,
    const QString &message)
{
    if (generation != generation_
        || (runtimeState_ != RuntimeState::Starting
            && runtimeState_ != RuntimeState::Running)) {
        return;
    }

    pendingModelReload_ = false;
    errorMessage_ = message;
    if (camera_ && autoRecoverCamera_)
        scheduleCameraRecovery(camera_->cameraDevice().id());
    requestSessionStop(
        RuntimeState::Faulted);
}

void RuntimeController::handleWorkerSessionFinished(
    std::uint64_t generation)
{
    if (generation != generation_) {
        return;
    }

    if (runtimeState_ == RuntimeState::Stopping) {
        finalizeSessionStop(generation);
    }
}

void RuntimeController::stop()
{
    // Explicit STOP always cancels both the retry policy and sticky intent.
    outputService_.stop();
    refreshOutputStatus();
    recoveryAttempt_ = 0;
    pendingModelReload_ = false;
    reloadQueued_ = false;
    stopGestureReplay(); // Global STOP also cancels offline replay.
    cancelCameraRecovery();
    requestSessionStop(
        RuntimeState::Stopped);
}

void RuntimeController::togglePipeline()
{
    if (canStop()) {
        stop();
    }
    else if (canStart()) {
        start();
    }
}

void RuntimeController::requestApplicationExit()
{
    if (exitRequested_) {
        return;
    }

    exitRequested_ = true;
    outputService_.stop();
    refreshOutputStatus();
    pendingModelReload_ = false;
    reloadQueued_ = false;
    stopGestureReplay();
    cancelCameraRecovery();
    emit applicationExitRequested();

    if (canStop()) {
        requestSessionStop(
            RuntimeState::Stopped);
        return;
    }

    if (runtimeState_ != RuntimeState::Stopping) {
        maybeEmitApplicationExitReady();
    }
}

void RuntimeController::maybeEmitApplicationExitReady()
{
    if (!exitRequested_) {
        return;
    }

    if (runtimeState_ != RuntimeState::Stopped
        && runtimeState_ != RuntimeState::Faulted) {
        return;
    }

    exitRequested_ = false;
    emit applicationExitReady();
}

void RuntimeController::workerLoop(
    std::stop_token stopToken)
{
    while (!stopToken.stop_requested()) {
        std::optional<WorkerSession> session;

        {
            std::unique_lock lock(workerMutex_);

            workerCondition_.wait(
                lock,
                [this, &stopToken] {
                    return stopToken.stop_requested()
                        || pendingWorkerSession_
                            .has_value();
                });

            if (stopToken.stop_requested()) {
                break;
            }

            session =
                std::move(
                    pendingWorkerSession_);

            pendingWorkerSession_.reset();

            if (session) {
                activeWorkerGeneration_ =
                    session->generation;
            }
        }

        if (!session || !session->inbox) {
            std::lock_guard lock(workerMutex_);
            activeWorkerGeneration_.reset();
            continue;
        }

        const std::uint64_t generation =
            session->generation;

        if (!session->inbox->isClosed()) {
            try {
                MediaPipeTracker tracker(
                    session->libraryPath,
                    session->modelPath,
                    session->detectionConfidence,
                    session->trackingConfidence);

                HandIdentityStabilizer stabilizer;
                bool previousSwap =
                    swapHandedness_.load();

                std::unique_ptr<OnnxGestureRecognizer> gestureModel;
                GestureEngine::ModelInference modelInference;
                QString gestureBackend = QStringLiteral("Rules");
                QString gestureModelStatus =
                    QStringLiteral("Rule fallback active");
                bool gestureModelActive = false;

#ifdef VC_WITH_ONNX
                if (QFileInfo::exists(session->gestureModelPath)) {
                    try {
                        gestureModel =
                            std::make_unique<OnnxGestureRecognizer>(
                                session->gestureModelPath);

                        if (gestureModel->windowSize() != 16) {
                            throw std::runtime_error(
                                "M3 runtime currently expects a 16-frame gesture window");
                        }

                        auto *model = gestureModel.get();
                        modelInference =
                            [model](const std::vector<float> &tensor) {
                                return model->infer(tensor);
                            };

                        gestureBackend = QStringLiteral("ONNX TCN");
                        gestureModelStatus =
                            QStringLiteral("gesture_model.onnx loaded");
                        gestureModelActive = true;
                    }
                    catch (const std::exception &error) {
                        qWarning()
                            << "[gesture] model rejected; using rules:"
                            << error.what();

                        gestureModel.reset();
                        modelInference = {};
                        gestureBackend = QStringLiteral("Rules");
                        gestureModelStatus =
                            QStringLiteral("Model rejected - rule fallback");
                        gestureModelActive = false;
                    }
                }
                else {
                    gestureModelStatus =
                        QStringLiteral("No gesture_model.onnx - rule fallback");
                }
#else
                if (QFileInfo::exists(session->gestureModelPath)) {
                    gestureModelStatus =
                        QStringLiteral("Model present, but this build has ONNX disabled");
                }
                else {
                    gestureModelStatus =
                        QStringLiteral("Rule fallback active (ONNX disabled)");
                }
#endif

                GestureEngine gestureEngine(
                    std::move(modelInference),
                    gestureBackend.toStdString());

                QMetaObject::invokeMethod(
                    this,
                    [this,
                     generation,
                     gestureBackend,
                     gestureModelStatus,
                     gestureModelActive] {
                        updateGestureBackendStatus(
                            gestureBackend,
                            gestureModelStatus,
                            gestureModelActive,
                            generation);
                    },
                    Qt::QueuedConnection);

                QMetaObject::invokeMethod(
                    this,
                    [this, generation] {
                        if (generation != generation_
                            || runtimeState_
                                   != RuntimeState::Starting) {
                            return;
                        }

                        trackerReady_ = true;
                        setRuntimeState(
                            RuntimeState::Running);
                    },
                    Qt::QueuedConnection);

                // Backpressure for *processed* results, not just camera frames.
                // We must never discard an already recognized gesture
                // transition on its way to ActionMapper/OutputService.
                auto guiGate = std::make_shared<GuiResultBackpressure>();
                while (!stopToken.stop_requested()
                       && !session->inbox->isClosed()) {
                    // Wait BEFORE inference. A congested GUI therefore
                    // causes capture frames to coalesce in LatestFrameSlot
                    // rather than queueing stale recognized gestures.
                    const auto waitingSinceUs = nowUs();
                    auto guiLease = guiGate->acquire(stopToken, [&] {
                        return session->inbox->isClosed();
                    });
                    if (!guiLease) break;
                    if (session->metrics)
                        session->metrics->recordGuiBackpressureWait(nowUs() - waitingSinceUs);

                    auto packet =
                        session->inbox->take(
                            std::chrono::milliseconds(25));

                    if (!packet) {
                        continue;
                    }

                    if (stopToken.stop_requested()
                        || session->inbox->isClosed()) {
                        break;
                    }

                    const auto captureUs =
                        packet->captureUs;
                    const auto sequence =
                        packet->sequence;

                    const auto conversionStartUs =
                        nowUs();

                    QImage image =
                        packet->frame
                            .toImage()
                            .convertToFormat(
                                QImage::Format_RGB888);

                    packet.reset();

                    if (image.isNull()) {
                        throw std::runtime_error(
                            "Cannot convert camera frame "
                            "to RGB888");
                    }

                    std::vector<std::uint8_t> rgb(
                        static_cast<std::size_t>(
                            image.width())
                        * static_cast<std::size_t>(
                            image.height())
                        * 3U);

                    for (int y = 0;
                         y < image.height();
                         ++y) {

                        std::memcpy(
                            rgb.data()
                                + static_cast<std::size_t>(y)
                                    * static_cast<std::size_t>(
                                        image.width())
                                    * 3U,
                            image.constScanLine(y),
                            static_cast<std::size_t>(
                                image.width())
                                * 3U);
                    }

                    const auto conversionEndUs =
                        nowUs();

                    const RawTrackingFrame raw =
                        tracker.process(
                            image.width(),
                            image.height(),
                            rgb,
                            captureUs,
                            sequence);

                    const bool currentSwap =
                        swapHandedness_.load();

                    if (currentSwap != previousSwap) {
                        stabilizer.reset();
                        gestureEngine.reset();
                        previousSwap = currentSwap;
                    }

                    const TrackingFrame tracking =
                        stabilizer.update(
                            raw,
                            currentSwap);

                    GestureThresholds thresholds;
                    thresholds.fallbackPercent = recognitionThreshold_.load();
                    for (std::size_t idx = 1; idx < kGestureClassCount; ++idx)
                        thresholds.overrides[idx] = gestureThresholdOverrides_[idx].load();
                    const auto gestureStartUs = nowUs();
                    for (std::size_t side = 0; side < kHandCount; ++side) {
                        gestureEngine.setPinchRange(static_cast<HandSide>(side),
                            appliedPinchLow_[side].load(), appliedPinchHigh_[side].load());
                    }
                    const GestureFrame gestures = gestureEngine.update(tracking, thresholds);
                    const auto gestureEndUs = nowUs();

                    const auto processingEndUs =
                        nowUs();

                    if (session->metrics) {
                        session->metrics
                            ->recordProcessed(
                                conversionEndUs
                                    - conversionStartUs,
                                raw.trackEndUs
                                    - raw.trackStartUs,
                                processingEndUs
                                    - captureUs,
                                gestureEndUs - gestureStartUs);
                    }

                    const auto postedUs = nowUs();
                    const bool enqueued = QMetaObject::invokeMethod(
                        this,
                        [this, tracking, gestures, generation, postedUs, guiLease] {
                            // The captured lease is released only when this
                            // callback finishes or Qt destroys an undelivered
                            // callback during shutdown. The gesture events
                            // cannot be overtaken by a newer result.
                            if (generation != generation_)
                                return;
                            if (metrics_)
                                metrics_->recordGuiQueueDelay(nowUs() - postedUs);
                            applyTrackingFrame(
                                tracking,
                                gestures,
                                generation);
                        },
                        Qt::QueuedConnection);
                    if (!enqueued)
                        throw std::runtime_error("Cannot queue tracking result to GUI");
                }
            }
            catch (const std::exception &error) {
                const QString message =
                    QString::fromUtf8(
                        error.what());

                QMetaObject::invokeMethod(
                    this,
                    [this, message, generation] {
                        handleWorkerFault(
                            generation,
                            message);
                    },
                    Qt::QueuedConnection);
            }
        }

        {
            std::lock_guard lock(workerMutex_);

            if (activeWorkerGeneration_
                    .has_value()
                && *activeWorkerGeneration_
                       == generation) {
                activeWorkerGeneration_.reset();
            }
        }

        QMetaObject::invokeMethod(
            this,
            [this, generation] {
                handleWorkerSessionFinished(
                    generation);
            },
            Qt::QueuedConnection);
    }

    qInfo() << "[shutdown] workerLoop exited";
}

QVariantList RuntimeController::toVariantLandmarks(
    const HandTrackingState &hand)
{
    QVariantList result;

    if (!hand.tracked) {
        return result;
    }

    result.reserve(
        static_cast<qsizetype>(
            kHandLandmarkCount));

    for (const auto &landmark : hand.landmarks) {
        QVariantMap point;
        point.insert(QStringLiteral("x"), landmark.x);
        point.insert(QStringLiteral("y"), landmark.y);
        point.insert(QStringLiteral("z"), landmark.z);
        result.append(point);
    }

    return result;
}

void RuntimeController::applyTrackingFrame(
    const TrackingFrame &frame,
    const GestureFrame &gestures,
    std::uint64_t generation)
{
    if (generation != generation_
        || runtimeState_ != RuntimeState::Running) {
        return;
    }

    const bool previousLeftTracked = leftTracked_;
    const bool previousRightTracked = rightTracked_;
    const QString previousLeftGesture = leftGesture_;
    const QString previousRightGesture = rightGesture_;
    const QString previousLeftEvent = leftGestureEvent_;
    const QString previousRightEvent = rightGestureEvent_;
    const ControllerState previousControllerState = controllerState_;

    leftTracked_ =
        frame.hands[handIndex(HandSide::Left)]
            .tracked;

    rightTracked_ =
        frame.hands[handIndex(HandSide::Right)]
            .tracked;

    leftHandednessConfidence_ =
        frame.hands[handIndex(HandSide::Left)]
            .handednessConfidence;

    rightHandednessConfidence_ =
        frame.hands[handIndex(HandSide::Right)]
            .handednessConfidence;

    const auto sideName = [](HandSide side) {
        return side == HandSide::Left
            ? QStringLiteral("L")
            : QStringLiteral("R");
    };

    leftReportedSide_ = leftTracked_
        ? sideName(
              frame.hands[handIndex(HandSide::Left)]
                  .reportedSide)
        : QStringLiteral("-");

    rightReportedSide_ = rightTracked_
        ? sideName(
              frame.hands[handIndex(HandSide::Right)]
                  .reportedSide)
        : QStringLiteral("-");

    const auto &leftGesture =
        gestures.hands[handIndex(HandSide::Left)];
    const auto &rightGesture =
        gestures.hands[handIndex(HandSide::Right)];

    leftGesture_ = leftGesture.ready
        ? gestureDisplayName(leftGesture.gesture)
        : QStringLiteral("NONE");
    rightGesture_ = rightGesture.ready
        ? gestureDisplayName(rightGesture.gesture)
        : QStringLiteral("NONE");
    leftGestureConfidence_ = leftGesture.ready
        ? leftGesture.confidence
        : 0.0;
    rightGestureConfidence_ = rightGesture.ready
        ? rightGesture.confidence
        : 0.0;

    GestureStateConfig eventConfig;
    eventConfig.debounceMs = debounceMs_;
    eventConfig.cooldownMs = cooldownMs_;
    eventConfig.requireRelease = requireRelease_;
    eventConfig.dropoutGraceMs = gestureGraceMs_;
    eventConfig.releaseThresholdPercent = gestureReleaseThreshold_;

    const GestureEventFrame events =
        gestureStateManager_.update(gestures, eventConfig);

    calibrator_.ingest(gestures);
    rangeCalibrator_.ingest(frame);

    leftGestureEvent_ = gestureEventDisplayName(
        events.hands[handIndex(HandSide::Left)].phase);
    rightGestureEvent_ = gestureEventDisplayName(
        events.hands[handIndex(HandSide::Right)].phase);

    if (recorder_.active()) {
        // Geometry validity has ALREADY been calculated by GestureEngine on
        // the worker thread. Reuse it: do not recompute it on the GUI thread.
        const std::array<bool, kHandCount> usableGeometry = {
            gestures.hands[0].geometryValid,
            gestures.hands[1].geometryValid
        };
        if (!recorder_.append(frame, usableGeometry)) {
            recordingError_ = QString::fromStdString(recorder_.error());
            lastRecordingQualitySignalUs_ = 0;
            emit recordingQualityChanged();
            emit recordingsChanged();
        } else if (lastRecordingQualitySignalUs_ == 0
                   || frame.captureUs < lastRecordingQualitySignalUs_
                   || frame.captureUs - lastRecordingQualitySignalUs_ >= 200'000) {
            // Updating the nested QML progress model on every incoming frame
            // needlessly triggers a full group of bindings/layout updates.
            lastRecordingQualitySignalUs_ = frame.captureUs;
            emit recordingQualityChanged();
        }
    }

    const auto outputStartUs = nowUs();
    updateContinuousControl(frame, events);
    const auto outputSubmittedUs = nowUs();
    if (metrics_) {
        metrics_->recordOutputDispatch(outputSubmittedUs - outputStartUs);
        metrics_->recordInputSubmitLatency(outputSubmittedUs - frame.captureUs);
    }

    // Keep GestureStateManager, recording and output at full processing FPS.
    // Only presentation QVariant conversion and QML notifications are paced.
    // Gesture/button/tracking edges are sent immediately for responsiveness.
    const bool eventEdge = previousLeftTracked != leftTracked_
        || previousRightTracked != rightTracked_
        || previousLeftGesture != leftGesture_
        || previousRightGesture != rightGesture_
        || previousLeftEvent != leftGestureEvent_
        || previousRightEvent != rightGestureEvent_
        || previousControllerState.mouseLeft != controllerState_.mouseLeft
        || previousControllerState.mouseRight != controllerState_.mouseRight
        || previousControllerState.buttons != controllerState_.buttons
        || previousControllerState.keys != controllerState_.keys
        || previousControllerState.cursorFrozen != controllerState_.cursorFrozen
        || previousControllerState.cursorMovementLocked != controllerState_.cursorMovementLocked;
    constexpr std::int64_t kUiIntervalUs = 33'000;  // max ~30 UI notifications/second
    const bool refreshUi = eventEdge || lastTrackingUiNotifyUs_ == 0
        || outputSubmittedUs < lastTrackingUiNotifyUs_
        || outputSubmittedUs - lastTrackingUiNotifyUs_ >= kUiIntervalUs;
    if (!refreshUi)
        return;

    leftLandmarks_ = toVariantLandmarks(frame.hands[handIndex(HandSide::Left)]);
    rightLandmarks_ = toVariantLandmarks(frame.hands[handIndex(HandSide::Right)]);
    resolution_ = QStringLiteral("%1 x %2").arg(frame.width).arg(frame.height);

    // Diagnostic QVariantMaps are not needed at full video cadence.
    if (eventEdge || lastDiagnosticsNotifyUs_ == 0
        || outputSubmittedUs < lastDiagnosticsNotifyUs_
        || outputSubmittedUs - lastDiagnosticsNotifyUs_ >= 100'000) {
        gestureDiagnostics_.clear();
        for (std::size_t index = 0; index < kHandCount; ++index) {
            const auto side = static_cast<HandSide>(index);
            gestureDiagnostics_.append(diagnosticMap(
                gestures.hands[index],
                gestureStateManager_.debugState(side, frame.captureUs)));
        }
        lastDiagnosticsNotifyUs_ = outputSubmittedUs;
    }

    lastTrackingUiNotifyUs_ = outputSubmittedUs;
    emit stateChanged();
}

void RuntimeController::updateGestureBackendStatus(
    const QString &backendName,
    const QString &modelStatus,
    bool modelActive,
    std::uint64_t generation)
{
    if (generation != generation_) {
        return;
    }

    gestureBackendName_ = backendName;
    gestureModelStatus_ = modelStatus;
    gestureModelActive_ = modelActive;
    emit stateChanged();
}

void RuntimeController::clearTracking()
{
    playgroundMode_ = false;
    cursorPlayground_.stop();
    calibrator_.cancel();
    rangeCalibrator_.cancel();
    stopGestureRecording();
    gestureDiagnostics_.clear();
    leftTracked_ = false;
    rightTracked_ = false;
    leftHandednessConfidence_ = 0.0;
    rightHandednessConfidence_ = 0.0;
    leftReportedSide_ = QStringLiteral("-");
    rightReportedSide_ = QStringLiteral("-");
    leftLandmarks_.clear();
    rightLandmarks_.clear();
    leftGesture_ = QStringLiteral("NONE");
    rightGesture_ = QStringLiteral("NONE");
    leftGestureConfidence_ = 0.0;
    rightGestureConfidence_ = 0.0;
    leftGestureEvent_ = QStringLiteral("IDLE");
    rightGestureEvent_ = QStringLiteral("IDLE");
    resetGestureActions();
    resetContinuousControl();
}

void RuntimeController::resetContinuousControl()
{
    continuousControl_.reset();
    controllerState_ = {};
}

void RuntimeController::resetGestureActions()
{
    gestureStateManager_.reset();
    actionMapper_.reset();
    leftGestureEvent_ = QStringLiteral("IDLE");
    rightGestureEvent_ = QStringLiteral("IDLE");

    // A gesture or tracking reset releases every held/toggled input while
    // preserving the explicit ON request. Only fresh frames can resume it.
    outputService_.pause("Gesture state reset - waiting for tracking");
    refreshOutputStatus();
}

void RuntimeController::updateCursorGeometry()
{
    const QScreen *screen =
        QGuiApplication::primaryScreen();

    if (!screen) {
        cursorScreenWidth_ = 1920;
        cursorScreenHeight_ = 1080;
        return;
    }

    const QSize size =
        screen->availableGeometry().size();

    cursorScreenWidth_ =
        std::max(size.width(), 1);
    cursorScreenHeight_ =
        std::max(size.height(), 1);
}

void RuntimeController::updateContinuousControl(
    const TrackingFrame &frame,
    const GestureEventFrame &events)
{
    ContinuousControlConfig config;
    config.sensitivity = sensitivity_;
    config.cursorSpeedPercent = cursorSpeedPercent_;
    config.deadzone = deadzone_;
    config.smoothing = smoothing_;
    config.invertX = invertX_;
    config.invertY = invertY_;
    config.movementScaleX = appliedMotionX_[handIndex(HandSide::Right)];
    config.movementScaleY = appliedMotionY_[handIndex(HandSide::Right)];

    const auto &rightHand =
        frame.hands[handIndex(HandSide::Right)];

    const auto result =
        continuousControl_.update(
            rightHand,
            frame.captureUs,
            config);

    controllerState_ = actionMapper_.apply(
        events,
        result.controller);

    if (cursorPlayground_.active()) {
        // Use *raw continuous motion* independently of the active gesture
        // mapping. Profile click actions remain observable, but nothing is
        // sent to the OS while the playground is active.
        playgroundSampleUs_ = frame.captureUs;
        cursorPlayground_.update(result.controller.mouseX,
                                 result.controller.mouseY,
                                 result.cursorTracked,
                                 controllerState_.mouseLeft,
                                 frame.captureUs);
    }

    if (result.cursorTracked) {
        if (!cursorPreviewInitialized_) {
            updateCursorGeometry();
            cursorX_ = cursorScreenWidth_ / 2;
            cursorY_ = cursorScreenHeight_ / 2;
            cursorAccumulatorX_ = static_cast<double>(cursorX_);
            cursorAccumulatorY_ = static_cast<double>(cursorY_);
            cursorPreviewInitialized_ = true;
        }

        cursorAccumulatorX_ = std::clamp(
            cursorAccumulatorX_
                + static_cast<double>(controllerState_.mouseX),
            0.0,
            static_cast<double>(
                std::max(cursorScreenWidth_ - 1, 0)));

        cursorAccumulatorY_ = std::clamp(
            cursorAccumulatorY_
                + static_cast<double>(controllerState_.mouseY),
            0.0,
            static_cast<double>(
                std::max(cursorScreenHeight_ - 1, 0)));

        cursorX_ = static_cast<int>(
            std::lround(cursorAccumulatorX_));
        cursorY_ = static_cast<int>(
            std::lround(cursorAccumulatorY_));
    }

    // The output guard ignores submissions while disarmed. When armed, even a
    // neutral frame is submitted so losing the right hand immediately stops
    // motion without requiring the output service to wait for its watchdog.
    if (!playgroundMode_)
        outputService_.submit(controllerState_, frame.captureUs, frame.sequence,
                              leftTracked_ || rightTracked_);
}

void RuntimeController::refreshOutputStatus()
{
    outputSnapshot_ =
        outputService_.status();
}

QString RuntimeController::rangeCalibrationStatus() const
{
    switch (rangeCalibrator_.status()) {
    case HandRangeCalibrator::Status::Capturing: return QStringLiteral("CAPTURING");
    case HandRangeCalibrator::Status::Ready: return QStringLiteral("READY");
    case HandRangeCalibrator::Status::Insufficient: return QStringLiteral("INSUFFICIENT");
    case HandRangeCalibrator::Status::Cancelled: return QStringLiteral("CANCELLED");
    default: return QStringLiteral("IDLE");
    }
}

QVariantList RuntimeController::rangeCalibrationResults() const
{
    QVariantList result;
    for (std::size_t i = 0; i < kHandCount; ++i) {
        const auto r = rangeCalibrator_.result(static_cast<HandSide>(i));
        QVariantMap item;
        item.insert(QStringLiteral("side"), i == 0 ? QStringLiteral("Left") : QStringLiteral("Right"));
        item.insert(QStringLiteral("motionValid"), r.motionValid);
        item.insert(QStringLiteral("spanX"), r.spanX);
        item.insert(QStringLiteral("spanY"), r.spanY);
        item.insert(QStringLiteral("closedValid"), r.closedValid);
        item.insert(QStringLiteral("closedDistance"), r.closedDistance);
        item.insert(QStringLiteral("openValid"), r.openValid);
        item.insert(QStringLiteral("openDistance"), r.openDistance);
        result.append(item);
    }
    return result;
}

QVariantMap RuntimeController::handCalibration() const
{
    QVariantMap values;
    for (std::size_t i = 0; i < kHandCount; ++i) {
        QVariantMap hand;
        hand.insert(QStringLiteral("motionX"), appliedMotionX_[i]);
        hand.insert(QStringLiteral("motionY"), appliedMotionY_[i]);
        hand.insert(QStringLiteral("pinchLow"), appliedPinchLow_[i].load());
        hand.insert(QStringLiteral("pinchHigh"), appliedPinchHigh_[i].load());
        values.insert(i == 0 ? QStringLiteral("Left") : QStringLiteral("Right"), hand);
    }
    return values;
}

void RuntimeController::setHandCalibration(const QVariantMap &values)
{
    bool changed = false;
    for (std::size_t i = 0; i < kHandCount; ++i) {
        const auto hand = values.value(i == 0 ? QStringLiteral("Left") : QStringLiteral("Right")).toMap();
        if (hand.isEmpty()) continue;
        const float x = hand.value(QStringLiteral("motionX"), 1.0).toFloat();
        const float y = hand.value(QStringLiteral("motionY"), 1.0).toFloat();
        const float low = hand.value(QStringLiteral("pinchLow"), 0.17).toFloat();
        const float high = hand.value(QStringLiteral("pinchHigh"), 0.43).toFloat();
        if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(low) || !std::isfinite(high)
            || x < 0.5F || x > 2.0F || y < 0.5F || y > 2.0F
            || low < 0.02F || high > 2.0F || high - low < 0.03F) continue;
        appliedMotionX_[i] = x;
        appliedMotionY_[i] = y;
        appliedPinchLow_[i].store(low);
        appliedPinchHigh_[i].store(high);
        changed = true;
    }
    if (changed) {
        resetContinuousControl();
        emit settingsChanged();
    }
}

bool RuntimeController::startRangeCalibration(const QString &side, const QString &kind)
{
    const auto parsedSide = parseHandName(side);
    if (!parsedSide || runtimeState_ != RuntimeState::Running) return false;
    HandRangeCalibrator::Kind mode;
    if (kind == QStringLiteral("Motion")) mode = HandRangeCalibrator::Kind::Motion;
    else if (kind == QStringLiteral("PinchClosed")) mode = HandRangeCalibrator::Kind::PinchClosed;
    else if (kind == QStringLiteral("PinchOpen")) mode = HandRangeCalibrator::Kind::PinchOpen;
    else return false;
    cancelGestureCalibration();
    stopCursorPlayground();
    setOutputArmed(false);
    rangeCalibrator_.start(*parsedSide, mode);
    emit stateChanged();
    return true;
}

void RuntimeController::cancelRangeCalibration()
{
    rangeCalibrator_.cancel();
    emit stateChanged();
}

bool RuntimeController::applyRangeCalibration(const QString &side, const QString &kind)
{
    const auto parsedSide = parseHandName(side);
    if (!parsedSide || rangeCalibrator_.capturing()) return false;
    const auto r = rangeCalibrator_.result(*parsedSide);
    QVariantMap values = handCalibration();
    auto hand = values.value(side).toMap();
    if (kind == QStringLiteral("Motion") && r.motionValid) {
        hand.insert(QStringLiteral("motionX"), std::clamp(0.35F / r.spanX, 0.5F, 2.0F));
        hand.insert(QStringLiteral("motionY"), std::clamp(0.35F / r.spanY, 0.5F, 2.0F));
    } else if (kind == QStringLiteral("Pinch") && r.closedValid && r.openValid
               && r.openDistance - r.closedDistance >= 0.12F) {
        const float span = r.openDistance - r.closedDistance;
        hand.insert(QStringLiteral("pinchLow"), r.closedDistance + 0.10F * span);
        hand.insert(QStringLiteral("pinchHigh"), r.closedDistance + 0.50F * span);
        if (hand.value(QStringLiteral("pinchHigh")).toFloat() > 2.0F
            || hand.value(QStringLiteral("pinchLow")).toFloat() < 0.02F)
            return false;
    } else return false;
    values.insert(side, hand);
    setHandCalibration(values);
    return true;
}

void RuntimeController::resetRangeCalibration()
{
    rangeCalibrator_.reset();
    QVariantMap defaults;
    for (const auto &name : {QStringLiteral("Left"), QStringLiteral("Right")}) {
        defaults.insert(name, QVariantMap{{QStringLiteral("motionX"), 1.0},
            {QStringLiteral("motionY"), 1.0}, {QStringLiteral("pinchLow"), 0.17},
            {QStringLiteral("pinchHigh"), 0.43}});
    }
    setHandCalibration(defaults);
    emit stateChanged();
}

bool RuntimeController::reloadGestureModel()
{
    if (runtimeState_ != RuntimeState::Running) {
        gestureModelStatus_ = QStringLiteral("Start tracking before reloading the model.");
        emit stateChanged();
        return false;
    }
    pendingModelReload_ = true;
    requestSessionStop(RuntimeState::Stopped);
    return true;
}

bool RuntimeController::startGestureReplay(const QString &filePath)
{
    stopGestureReplay();
    replayError_.clear();
    replayStatus_.clear();
    if (pipelineRunning()) {
        replayError_ = QStringLiteral("Stop tracking before playing a recording.");
        emit stateChanged();
        return false;
    }
    // Fail closed even if a previously running pipeline left a native button
    // latched. This replay path never calls OutputService::arm or submit.
    outputService_.stop();
    actionMapper_.reset();
    refreshOutputStatus();
    if (!replayReader_.open(filePath)) {
        replayError_ = replayReader_.error();
        emit stateChanged();
        return false;
    }
    replayEngine_.reset();
    replayStateManager_.reset();
    replayFrames_ = 0;
    replayGestures_ = {QStringLiteral("NONE"), QStringLiteral("NONE")};
    replayLabels_ = {QStringLiteral("UNLABELED"), QStringLiteral("UNLABELED")};
    replayStatus_ = QStringLiteral("Rules playback (offline, output disabled)");
    replayTimer_.start();
    emit stateChanged();
    return true;
}

void RuntimeController::stopGestureReplay()
{
    const bool wasPlaying = replayTimer_.isActive();
    replayTimer_.stop();
    replayReader_.close();
    if (wasPlaying)
        replayStatus_ = QStringLiteral("Playback stopped");
    // Keep the last result on screen until a new session starts.
    emit stateChanged();
}

void RuntimeController::tickGestureReplay()
{
    TrackingFrame frame;
    std::array<QString, kHandCount> labels;
    if (!replayReader_.next(&frame, &labels)) {
        const QString failure = replayReader_.error();
        stopGestureReplay();
        replayError_ = failure;
        replayStatus_ = failure.isEmpty()
            ? QStringLiteral("Playback complete")
            : QStringLiteral("Playback stopped: invalid recording");
        emit stateChanged();
        return;
    }
    GestureThresholds thresholds;
    thresholds.fallbackPercent = recognitionThreshold_.load();
    for (std::size_t idx = 1; idx < kGestureClassCount; ++idx)
        thresholds.overrides[idx] = gestureThresholdOverrides_[idx].load();
    for (std::size_t side = 0; side < kHandCount; ++side) {
        replayEngine_.setPinchRange(static_cast<HandSide>(side),
            appliedPinchLow_[side].load(), appliedPinchHigh_[side].load());
    }
    const GestureFrame gestures = replayEngine_.update(frame, thresholds);
    replayStateManager_.update(gestures, {}); // state evolution only; no actions
    for (std::size_t side = 0; side < kHandCount; ++side) {
        const auto &prediction = gestures.hands[side];
        replayGestures_[side] = prediction.ready
            ? gestureDisplayName(prediction.gesture) : QStringLiteral("NONE");
    }
    replayLabels_ = labels;
    ++replayFrames_;
    emit stateChanged();
}

void RuntimeController::updateStats()
{
    refreshOutputStatus();
    if (!metrics_ || !pipelineRunning()) {
        emit stateChanged();
        return;
    }

    const auto timestamp = nowUs();
    metricsSnapshot_ = metrics_->snapshot(timestamp);
    const auto lastFrameUs = metricsSnapshot_.lastFrameReceivedUs;
    const auto referenceUs = lastFrameUs > 0 ? lastFrameUs : sessionStartedUs_;
    const std::int64_t frameAgeUs = referenceUs > 0 ? timestamp - referenceUs : 0;

    if (frameAgeUs > 500'000 && !captureStallHandled_) {
        captureStallHandled_ = true;
        // Old gestures and held OS buttons cannot survive stalled capture.
        // The ON preference stays latched and can resume from fresh frames.
        clearTracking();
    }
    else if (frameAgeUs <= 500'000) {
        captureStallHandled_ = false;
        // A sustained healthy session resets fault backoff.
        if (referenceUs > 0 && timestamp - sessionStartedUs_ > 5'000'000)
            recoveryAttempt_ = 0;
    }

    // Some Windows camera drivers stall without emitting errorOccurred or
    // activeChanged. Recover after 3s of missing frames, at most once per
    // session; the camera reconnect policy throttles subsequent attempts.
    if (frameAgeUs > 3'000'000 && !cameraReconnectPolicy_.pending()
        && runtimeState_ == RuntimeState::Running && camera_
        && autoRecoverCamera_) {
        scheduleCameraRecovery(camera_->cameraDevice().id());
        requestSessionStop(RuntimeState::Faulted);
    }
    emit stateChanged();
}

} // namespace vc
