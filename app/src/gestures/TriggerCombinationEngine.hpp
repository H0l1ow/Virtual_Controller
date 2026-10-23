#pragma once

#include "gestures/GestureStateManager.hpp"
#include "mapping/MappingTypes.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace vc {

struct ResolvedTriggerEvent {
    TriggerKind kind{TriggerKind::Single};
    GestureEventPhase phase{GestureEventPhase::None};
    HandSide hand{HandSide::Right};
    GestureClass gesture{GestureClass::None};
    std::string combinationId;
    std::uint64_t trackId{};
};

struct ResolvedTriggerFrame {
    std::int64_t timestampUs{};
    bool hasTrackingStatus{};
    std::array<bool, kHandCount> tracked{};
    std::array<std::uint64_t, kHandCount> trackIds{};
    std::vector<ResolvedTriggerEvent> triggers;
};

// All arbitration precedes ActionMapper, including delaying singleton PRESS.
// Timestamps come from the tracking stream, not the QML refresh rate.
class TriggerCombinationEngine final
{
public:
    void setMappings(const std::vector<MappingRule> &mappings);
    ResolvedTriggerFrame update(const GestureEventFrame &events);
    void reset();

private:
    struct HandState {
        GestureClass gesture{GestureClass::None};
        std::uint64_t trackId{};
        std::int64_t sinceUs{};
        bool active{};
        bool emitted{};
        bool blocked{}; // a combo consumed this activation
    };

    const MappingRule *matchingPair() const;
    bool waitForPossiblePair(std::size_t index, std::int64_t timestampUs) const;
    static HandSide sideFor(std::size_t index);
    static void emitSingle(ResolvedTriggerFrame &frame, std::size_t index,
                           const HandState &hand, GestureEventPhase phase);
    void emitPair(ResolvedTriggerFrame &frame, GestureEventPhase phase) const;

    std::vector<MappingRule> combinations_;
    std::array<HandState, kHandCount> hands_{};
    std::string activePairId_;
    std::int64_t previousTimestampUs_{};
};

} // namespace vc
