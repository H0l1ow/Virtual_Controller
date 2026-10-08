#pragma once

#include "control/ControllerState.hpp"
#include "gestures/GestureStateManager.hpp"
#include "mapping/MappingTypes.hpp"

#include <set>
#include <string>
#include <vector>

namespace vc {

class ActionMapper final
{
public:
    void setMappings(std::vector<MappingRule> mappings);
    const std::vector<MappingRule> &mappings() const { return mappings_; }

    ControllerState apply(
        const GestureEventFrame &events,
        ControllerState baseState);

    void reset();

private:
    static bool matches(
        const MappingRule &rule,
        HandSide hand,
        const GestureEvent &event);

    static void applyAction(
        ControllerState &state,
        LogicalAction action);

    void applyPersistentStates(ControllerState &state) const;

    std::vector<MappingRule> mappings_;
    std::set<std::string> held_;
    std::set<std::string> toggled_;
};

} // namespace vc
