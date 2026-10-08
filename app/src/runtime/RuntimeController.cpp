#include "runtime/RuntimeController.hpp"

#include "gestures/GestureEngine.hpp"
#include "gestures/OnnxGestureRecognizer.hpp"
#include "tracking/HandIdentityStabilizer.hpp"
#include "tracking/MediaPipeTracker.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QDebug>
#include <QGuiApplication>
#include <QFileInfo>
#include <QImage>
#include <QMetaObject>
#include <QScreen>
#include <QStandardPaths>
#include <QVariantMap>
#include <QFileInfo>
#include <QVideoFrameFormat>
#include <QVideoSink>
#include <QtGlobal>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <exception>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <optional>

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
    , captureSession_(this)
    , statsTimer_(this)
{
    refreshDevices();
    updateCursorGeometry();
    cursorX_ = cursorScreenWidth_ / 2;
    cursorY_ = cursorScreenHeight_ / 2;
    cursorAccumulatorX_ = static_cast<double>(cursorX_);
    cursorAccumulatorY_ = static_cast<double>(cursorY_);

    initializeProfiles();

    connect(
        &mediaDevices_,
        &QMediaDevices::videoInputsChanged,
        this,
        [this] {
            if (pipelineRunning()) {
                requestSessionStop(
                    RuntimeState::Stopped);
            }

            refreshDevices();
        });

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

    captureSession_.setVideoOutput(nullptr);

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

    if (outputSnapshot_.armed) {
        outputService_.stop();
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
    if (outputSnapshot_.armed) {
        outputService_.stop();
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

    if (outputSnapshot_.armed) {
        outputService_.stop();
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

    if (outputSnapshot_.armed) {
        outputService_.stop();
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
    outputService_.stop();
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

void RuntimeController::setOutputArmed(
    bool value)
{
    refreshOutputStatus();

    if (outputSnapshot_.armed == value) {
        return;
    }

    if (value) {
        if (runtimeState_ != RuntimeState::Running
            || !outputSnapshot_.ready) {
            emit stateChanged();
            return;
        }

        // Re-prime continuous control when output becomes live. The next hand
        // frame therefore establishes a baseline instead of moving the mouse.
        resetContinuousControl();
        actionMapper_.reset();
        outputService_.arm();
    }
    else {
        outputService_.stop();
        actionMapper_.reset();
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

void RuntimeController::attachVideoOutput(
    QObject *output)
{
    if (!output) {
        if (pipelineRunning()) {
            requestSessionStop(
                RuntimeState::Stopped);
        }

        videoOutput_.clear();
        captureSession_.setVideoOutput(nullptr);
        return;
    }

    if (pipelineRunning()
        && videoOutput_ != output) {

        errorMessage_ =
            QStringLiteral(
                "Stop tracking before replacing "
                "the video output.");

        emit stateChanged();
        return;
    }

    videoOutput_ = output;
    captureSession_.setVideoOutput(output);
}

void RuntimeController::selectCamera(
    int index)
{
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

    errorMessage_.clear();

    if (cameras_.isEmpty()) {
        failWithoutSession(
            QStringLiteral(
                "No camera detected. "
                "Check Windows camera permissions."));
        return;
    }

    if (!videoOutput_) {
        failWithoutSession(
            QStringLiteral(
                "Camera preview is not attached yet."));
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

    auto *videoSink =
        captureSession_.videoSink();

    if (!videoSink) {
        failWithoutSession(
            QStringLiteral(
                "VideoOutput did not expose "
                "a QVideoSink."));
        return;
    }

    outputService_.stop();
    refreshOutputStatus();
    clearTracking();
    resetContinuousControl();
    metricsSnapshot_ = {};

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
                            requestSessionStop(
                                RuntimeState::Faulted);
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

    // System input is neutralized immediately on every explicit STOP/fault,
    // before native tracker teardown begins.
    outputService_.stop();
    refreshOutputStatus();
    resetContinuousControl();

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

    errorMessage_ = message;
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

                while (!stopToken.stop_requested()
                       && !session->inbox->isClosed()) {

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
                                QImage::Format_RGB888)
                            .copy();

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

                    const GestureFrame gestures =
                        gestureEngine.update(
                            tracking,
                            recognitionThreshold_.load());

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
                                    - captureUs);
                    }

                    QMetaObject::invokeMethod(
                        this,
                        [this, tracking, gestures, generation] {
                            applyTrackingFrame(
                                tracking,
                                gestures,
                                generation);
                        },
                        Qt::QueuedConnection);
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

    leftLandmarks_ =
        toVariantLandmarks(
            frame.hands[
                handIndex(HandSide::Left)]);

    rightLandmarks_ =
        toVariantLandmarks(
            frame.hands[
                handIndex(HandSide::Right)]);

    resolution_ =
        QStringLiteral("%1 x %2")
            .arg(frame.width)
            .arg(frame.height);

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

    const GestureEventFrame events =
        gestureStateManager_.update(gestures, eventConfig);

    leftGestureEvent_ = gestureEventDisplayName(
        events.hands[handIndex(HandSide::Left)].phase);
    rightGestureEvent_ = gestureEventDisplayName(
        events.hands[handIndex(HandSide::Right)].phase);

    updateContinuousControl(frame, events);
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

    // Changing event semantics while a key/button is held must never leave the
    // OS state latched. Disarm the output path; the user explicitly re-arms it.
    refreshOutputStatus();
    if (outputSnapshot_.armed) {
        outputService_.stop();
        refreshOutputStatus();
    }
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
    outputService_.submit(
        controllerState_,
        frame.captureUs,
        frame.sequence);
}

void RuntimeController::refreshOutputStatus()
{
    outputSnapshot_ =
        outputService_.status();
}

void RuntimeController::updateStats()
{
    refreshOutputStatus();

    if (!metrics_ || !pipelineRunning()) {
        emit stateChanged();
        return;
    }

    metricsSnapshot_ =
        metrics_->snapshot(nowUs());

    const auto lastFrameUs =
        metricsSnapshot_.lastFrameReceivedUs;

    if (lastFrameUs > 0
        && nowUs() - lastFrameUs > 500000) {
        clearTracking();
    }

    emit stateChanged();
}

} // namespace vc
