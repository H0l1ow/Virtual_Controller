#pragma once

#include "tracking/TrackingTypes.hpp"

#include <array>
#include <cstdint>

namespace vc {

class HandIdentityStabilizer final
{
public:
    struct Config {
        // How long a missing hand may be re-associated with its previous
        // stable identity instead of receiving a new trackId.
        std::int64_t reacquireWindowUs{350000};

        // Association is primarily spatial. A conflicting MediaPipe
        // handedness label adds this penalty, scaled by handedness confidence.
        float handednessMismatchPenalty{0.32F};

        // A large jump is still allowed (hands can move quickly), but receives
        // an extra cost so a nearby historical identity wins when available.
        float softDistanceLimit{0.42F};
        float farDistancePenalty{0.45F};

        // Exponential smoothing for the derived velocity.
        float velocitySmoothing{0.65F};
    };

    HandIdentityStabilizer();
    explicit HandIdentityStabilizer(Config config);

    TrackingFrame update(
        const RawTrackingFrame &frame,
        bool swapHandedness);

    void reset();

private:
    struct Slot {
        bool hasHistory{};
        HandSide side{HandSide::Left};
        Landmark palm{};
        Landmark velocity{};
        std::uint64_t trackId{};
        std::uint32_t ageFrames{};
        std::int64_t lastSeenUs{};
    };

    struct Candidate {
        const RawHandDetection *detection{};
        HandSide effectiveReportedSide{HandSide::Left};
        Landmark palm{};
    };

    float associationCost(
        const Slot &slot,
        const Candidate &candidate,
        std::int64_t captureUs) const;

    void assign(
        Slot &slot,
        const Candidate &candidate,
        std::int64_t captureUs,
        HandTrackingState &output);

    void emitMissing(
        Slot &slot,
        std::int64_t captureUs,
        HandTrackingState &output);

    static Landmark palmCenter(
        const std::array<Landmark, kHandLandmarkCount> &landmarks);

    Config config_;
    std::array<Slot, kHandCount> slots_{};
    std::uint64_t nextTrackId_{1};
};

} // namespace vc
