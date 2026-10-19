#pragma once

namespace vc {

// The capture sink feeds MediaPipe independently of presentation. There is
// never more than one Qt Quick preview target for a frame.
enum class PreviewTarget { None, Controller, Recording, PiP };

constexpr PreviewTarget selectPreviewTarget(bool controllerAttached,
                                            bool recordingAttached,
                                            bool pipAttached,
                                            bool minimizedPiPActive,
                                            bool mainWindowMinimized = false) noexcept
{
    // Minimize is a presentation-only event: never stop camera capture,
    // tracking, recording, or the user's requested system output.
    // While minimized, only the PiP renderer may receive video frames.
    if (mainWindowMinimized || minimizedPiPActive)
        return (minimizedPiPActive && pipAttached)
            ? PreviewTarget::PiP : PreviewTarget::None;
    if (recordingAttached)
        return PreviewTarget::Recording;
    if (controllerAttached)
        return PreviewTarget::Controller;
    return PreviewTarget::None;
}

} // namespace vc
