#pragma once

#include "gestures/GestureTypes.hpp"
#include "tracking/TrackingTypes.hpp"

#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>

namespace vc {

// Explicit opt-in: writes landmarks ONLY (never video/audio) to JSON Lines.
// A label is associated with a logical side for the whole session; unlabelled
// hands are still recorded so two-hand motion can be analyzed later.
class GestureDataRecorder final
{
public:
    using Labels = std::array<std::optional<GestureClass>, kHandCount>;
    static constexpr std::size_t kMaxFrames = 18000;

    bool start(const std::filesystem::path &directory, Labels labels);
    bool append(const TrackingFrame &frame);
    void stop();

    bool active() const { return stream_.is_open(); }
    std::size_t frameCount() const { return frameCount_; }
    // 'Usable' means labelled, tracked and numerically valid. It does NOT
    // verify that the operator chose the correct gesture label.
    std::size_t usableFrames(HandSide side) const { return usable_[handIndex(side)]; }
    std::size_t missingFrames(HandSide side) const { return missing_[handIndex(side)]; }
    bool labelled(HandSide side) const { return labels_[handIndex(side)].has_value(); }
    std::optional<GestureClass> gestureLabel(HandSide side) const { return labels_[handIndex(side)]; }
    static constexpr std::size_t kSuggestedFramesPerHand = 150;
    const std::filesystem::path &path() const { return path_; }
    const std::string &error() const { return error_; }

private:
    std::ofstream stream_;
    std::filesystem::path path_;
    std::string error_;
    Labels labels_{};
    std::size_t frameCount_{};
    std::array<std::size_t, kHandCount> usable_{};
    std::array<std::size_t, kHandCount> missing_{};
};

} // namespace vc
