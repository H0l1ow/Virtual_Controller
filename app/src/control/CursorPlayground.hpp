#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace vc {

// Qt/OS-independent virtual 960x540 plane. Never generates system input.
// Targets can be reached by holding the cursor inside for 550 ms; optional
// logical mouse presses are counted as hits/misses for profile diagnostics.
class CursorPlayground final {
public:
    struct Point { double x{}, y{}; };
    static constexpr double kWidth = 960.0;
    static constexpr double kHeight = 540.0;
    static constexpr double kRadius = 35.0;
    static constexpr std::array<Point, 6> kTargets{{
        {190, 125}, {755, 120}, {748, 405},
        {232, 405}, {492, 267}, {840, 270}
    }};

    void start(std::int64_t timestampUs);
    void stop();
    void update(double dx, double dy, bool tracked, bool logicalPressed,
                std::int64_t timestampUs);

    bool active() const { return active_; }
    bool complete() const { return complete_; }
    bool tracked() const { return tracked_; }
    int targetIndex() const { return targetIndex_; }
    int completedTargets() const { return targetIndex_; }
    int totalTargets() const { return static_cast<int>(kTargets.size()); }
    int missedClicks() const { return missedClicks_; }
    Point cursor() const { return position_; }
    Point target() const { return kTargets[static_cast<std::size_t>(
        targetIndex_ >= totalTargets() ? totalTargets()-1 : targetIndex_)]; }
    double averageAcquisitionMs() const { return targetIndex_ > 0
        ? acquisitionMs_ / targetIndex_ : 0.0; }
    double averageErrorPx() const { return targetIndex_ > 0
        ? errorPx_ / targetIndex_ : 0.0; }
    double averageJitterPx() const { return jitterSamples_ > 0
        ? jitterPx_ / jitterSamples_ : 0.0; }
    int dwellPercent(std::int64_t nowUs) const;

private:
    void completeTarget(std::int64_t timestampUs, double distance);
    bool active_{};
    bool complete_{};
    bool tracked_{};
    bool lastPressed_{};
    int targetIndex_{};
    int missedClicks_{};
    Point position_{kWidth/2.0, kHeight/2.0};
    Point lastPosition_{};
    bool lastPositionValid_{};
    std::int64_t targetStartUs_{};
    std::int64_t insideSinceUs_{};
    std::int64_t lastUs_{};
    double acquisitionMs_{};
    double errorPx_{};
    double jitterPx_{};
    int jitterSamples_{};
};

} // namespace vc
