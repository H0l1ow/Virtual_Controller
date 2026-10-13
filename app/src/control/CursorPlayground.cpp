#include "control/CursorPlayground.hpp"

#include <algorithm>
#include <cmath>

namespace vc {

void CursorPlayground::start(std::int64_t timestampUs)
{
    *this = CursorPlayground{};
    active_ = true;
    targetStartUs_ = timestampUs;
}

void CursorPlayground::stop()
{
    active_ = false;
    tracked_ = false;
    insideSinceUs_ = 0;
}

int CursorPlayground::dwellPercent(std::int64_t nowUs) const
{
    if (!active_ || !insideSinceUs_ || nowUs < insideSinceUs_)
        return 0;
    return std::clamp(static_cast<int>((nowUs - insideSinceUs_) / 5500), 0, 100);
}

void CursorPlayground::completeTarget(std::int64_t timestampUs, double distance)
{
    acquisitionMs_ += std::max<std::int64_t>(0, timestampUs - targetStartUs_) / 1000.0;
    errorPx_ += distance;
    ++targetIndex_;
    insideSinceUs_ = 0;
    lastPositionValid_ = false;
    targetStartUs_ = timestampUs;
    if (targetIndex_ == totalTargets()) {
        complete_ = true;
        active_ = false;
    }
}

void CursorPlayground::update(double dx, double dy, bool tracked,
                              bool logicalPressed, std::int64_t timestampUs)
{
    if (!active_ || timestampUs <= 0 || (lastUs_ && timestampUs <= lastUs_))
        return;
    lastUs_ = timestampUs;
    tracked_ = tracked;

    if (tracked && std::isfinite(dx) && std::isfinite(dy)) {
        position_.x = std::clamp(position_.x + std::clamp(dx, -200.0, 200.0), 0.0, kWidth);
        position_.y = std::clamp(position_.y + std::clamp(dy, -200.0, 200.0), 0.0, kHeight);
    }
    const Point goal = target();
    const double distance = std::hypot(position_.x-goal.x, position_.y-goal.y);
    const bool inside = tracked && distance <= kRadius;
    const bool click = tracked && logicalPressed && !lastPressed_;
    lastPressed_ = logicalPressed;

    if (inside) {
        if (insideSinceUs_ == 0) insideSinceUs_ = timestampUs;
        if (lastPositionValid_) {
            jitterPx_ += std::hypot(position_.x-lastPosition_.x,
                                    position_.y-lastPosition_.y);
            ++jitterSamples_;
        }
        lastPosition_ = position_;
        lastPositionValid_ = true;
    } else {
        insideSinceUs_ = 0;
        lastPositionValid_ = false;
    }

    if (click && !inside)
        ++missedClicks_;
    if (inside && (click || timestampUs - insideSinceUs_ >= 550000))
        completeTarget(timestampUs, distance);
}

} // namespace vc
