#include "tracking/HandIdentityStabilizer.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace vc {

namespace {

float distance2D(
    const Landmark &a,
    const Landmark &b)
{
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;

    return std::sqrt(dx * dx + dy * dy);
}

Landmark addScaled(
    const Landmark &position,
    const Landmark &velocity,
    float seconds)
{
    return {
        position.x + velocity.x * seconds,
        position.y + velocity.y * seconds,
        position.z + velocity.z * seconds
    };
}

} // namespace

HandIdentityStabilizer::HandIdentityStabilizer()
    : HandIdentityStabilizer(Config{})
{
}

HandIdentityStabilizer::HandIdentityStabilizer(
    Config config)
    : config_(config)
{
    reset();
}

void HandIdentityStabilizer::reset()
{
    slots_ = {};

    slots_[0].side = HandSide::Left;
    slots_[1].side = HandSide::Right;

    nextTrackId_ = 1;
}

Landmark HandIdentityStabilizer::palmCenter(
    const std::array<Landmark, kHandLandmarkCount> &landmarks)
{
    // Wrist + four MCP joints give a stable center that is less sensitive to
    // finger articulation than a fingertip-based point.
    constexpr std::array<std::size_t, 5> indices{
        0U, 5U, 9U, 13U, 17U
    };

    Landmark result;

    for (const auto index : indices) {
        result.x += landmarks[index].x;
        result.y += landmarks[index].y;
        result.z += landmarks[index].z;
    }

    constexpr float divisor =
        static_cast<float>(indices.size());

    result.x /= divisor;
    result.y /= divisor;
    result.z /= divisor;

    return result;
}

float HandIdentityStabilizer::associationCost(
    const Slot &slot,
    const Candidate &candidate,
    std::int64_t captureUs) const
{
    const bool recent =
        slot.hasHistory
        && captureUs >= slot.lastSeenUs
        && captureUs - slot.lastSeenUs
               <= config_.reacquireWindowUs;

    float cost = 0.0F;

    if (recent) {
        const float dtSeconds =
            std::clamp(
                static_cast<float>(
                    captureUs - slot.lastSeenUs)
                    / 1000000.0F,
                0.0F,
                0.35F);

        const Landmark predicted =
            addScaled(
                slot.palm,
                slot.velocity,
                dtSeconds);

        const float spatialDistance =
            distance2D(
                predicted,
                candidate.palm);

        cost += spatialDistance;

        if (spatialDistance
            > config_.softDistanceLimit) {
            cost += config_.farDistancePenalty;
        }
    }
    else {
        // With no recent history, handedness should dominate initial
        // assignment. A small fixed cost keeps the comparison deterministic.
        cost += 0.05F;
    }

    if (candidate.effectiveReportedSide
        != slot.side) {
        const float confidence =
            std::clamp(
                candidate.detection
                    ->handednessConfidence,
                0.0F,
                1.0F);

        cost +=
            config_.handednessMismatchPenalty
            * (0.6F + 0.8F * confidence);

        if (!recent) {
            // At startup/reacquisition, there is no spatial history to resolve
            // ambiguity, so trust a confident MediaPipe label more strongly.
            cost += 0.35F * confidence;
        }
    }

    return cost;
}

void HandIdentityStabilizer::assign(
    Slot &slot,
    const Candidate &candidate,
    std::int64_t captureUs,
    HandTrackingState &output)
{
    const bool recent =
        slot.hasHistory
        && captureUs >= slot.lastSeenUs
        && captureUs - slot.lastSeenUs
               <= config_.reacquireWindowUs;

    Landmark velocity{};

    if (recent && captureUs > slot.lastSeenUs) {
        const float dtSeconds =
            static_cast<float>(
                captureUs - slot.lastSeenUs)
            / 1000000.0F;

        if (dtSeconds > 0.0001F) {
            const Landmark instantaneous{
                (candidate.palm.x - slot.palm.x) / dtSeconds,
                (candidate.palm.y - slot.palm.y) / dtSeconds,
                (candidate.palm.z - slot.palm.z) / dtSeconds
            };

            const float keep =
                std::clamp(
                    config_.velocitySmoothing,
                    0.0F,
                    1.0F);

            const float take = 1.0F - keep;

            velocity = {
                slot.velocity.x * keep + instantaneous.x * take,
                slot.velocity.y * keep + instantaneous.y * take,
                slot.velocity.z * keep + instantaneous.z * take
            };
        }
    }

    if (!recent) {
        slot.trackId = nextTrackId_++;
        slot.ageFrames = 0;
    }

    slot.hasHistory = true;
    slot.palm = candidate.palm;
    slot.velocity = velocity;
    slot.lastSeenUs = captureUs;
    ++slot.ageFrames;

    output.tracked = true;
    output.side = slot.side;
    output.reportedSide =
        candidate.effectiveReportedSide;
    output.handednessConfidence =
        candidate.detection
            ->handednessConfidence;
    output.landmarks =
        candidate.detection
            ->landmarks;
    output.palmPosition = slot.palm;
    output.velocity = slot.velocity;
    output.trackId = slot.trackId;
    output.ageFrames = slot.ageFrames;
    output.lastSeenUs = slot.lastSeenUs;
}

void HandIdentityStabilizer::emitMissing(
    Slot &slot,
    std::int64_t captureUs,
    HandTrackingState &output)
{
    if (slot.hasHistory
        && captureUs >= slot.lastSeenUs
        && captureUs - slot.lastSeenUs
               > config_.reacquireWindowUs) {

        // The identity memory has expired. Keep side metadata but do not keep
        // an old trackId alive indefinitely.
        slot.hasHistory = false;
        slot.palm = {};
        slot.velocity = {};
        slot.trackId = 0;
        slot.ageFrames = 0;
        slot.lastSeenUs = 0;
    }

    output = {};
    output.side = slot.side;
    output.reportedSide = slot.side;
    output.trackId = slot.trackId;
    output.ageFrames = slot.ageFrames;
    output.lastSeenUs = slot.lastSeenUs;
    output.palmPosition = slot.palm;
    output.velocity = slot.velocity;
}

TrackingFrame HandIdentityStabilizer::update(
    const RawTrackingFrame &frame,
    bool swapHandedness)
{
    TrackingFrame output;

    output.width = frame.width;
    output.height = frame.height;
    output.captureUs = frame.captureUs;
    output.trackStartUs = frame.trackStartUs;
    output.trackEndUs = frame.trackEndUs;
    output.sequence = frame.sequence;

    std::array<Candidate, kHandCount> candidates{};
    std::size_t candidateCount = 0;

    for (const auto &detection : frame.detections) {
        if (!detection.detected) {
            continue;
        }

        auto &candidate =
            candidates[candidateCount++];

        candidate.detection = &detection;
        candidate.effectiveReportedSide =
            swapHandedness
            ? oppositeHandSide(
                  detection.reportedSide)
            : detection.reportedSide;
        candidate.palm =
            palmCenter(detection.landmarks);

        if (candidateCount == kHandCount) {
            break;
        }
    }

    std::array<int, kHandCount> assignment{-1, -1};

    if (candidateCount == 1) {
        const float leftCost =
            associationCost(
                slots_[0],
                candidates[0],
                frame.captureUs);

        const float rightCost =
            associationCost(
                slots_[1],
                candidates[0],
                frame.captureUs);

        assignment[
            leftCost <= rightCost ? 0U : 1U] = 0;
    }
    else if (candidateCount == 2) {
        const float directCost =
            associationCost(
                slots_[0],
                candidates[0],
                frame.captureUs)
            + associationCost(
                slots_[1],
                candidates[1],
                frame.captureUs);

        const float crossedCost =
            associationCost(
                slots_[0],
                candidates[1],
                frame.captureUs)
            + associationCost(
                slots_[1],
                candidates[0],
                frame.captureUs);

        if (directCost <= crossedCost) {
            assignment[0] = 0;
            assignment[1] = 1;
        }
        else {
            assignment[0] = 1;
            assignment[1] = 0;
        }
    }

    for (std::size_t slotIndex = 0;
         slotIndex < kHandCount;
         ++slotIndex) {

        if (assignment[slotIndex] >= 0) {
            assign(
                slots_[slotIndex],
                candidates[
                    static_cast<std::size_t>(
                        assignment[slotIndex])],
                frame.captureUs,
                output.hands[slotIndex]);
        }
        else {
            emitMissing(
                slots_[slotIndex],
                frame.captureUs,
                output.hands[slotIndex]);
        }
    }

    return output;
}

} // namespace vc
