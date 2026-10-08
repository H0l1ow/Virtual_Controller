#pragma once

#include "gestures/GestureTypes.hpp"
#include "tracking/TrackingTypes.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace vc {

enum class ActionBehavior : std::uint8_t {
    Press = 0,
    Hold,
    Toggle
};

enum class LogicalAction : std::uint8_t {
    MouseLeft = 0,
    MouseRight,
    ScrollUp,
    ScrollDown,
    CursorFreeze,
    KeySpace,
    KeyEnter,
    KeyEscape,
    KeyTab,
    KeyLeft,
    KeyRight,
    KeyUp,
    KeyDown,
    KeyCtrl,
    KeyShift,
    KeyAlt
};

struct MappingRule {
    std::string id;
    HandSide hand{HandSide::Right};
    GestureClass gesture{GestureClass::Pinch};
    LogicalAction action{LogicalAction::MouseLeft};
    ActionBehavior behavior{ActionBehavior::Hold};
    bool enabled{true};
};

struct MappingProfile {
    std::string name{"Default"};
    std::vector<MappingRule> mappings;
};

inline const char *actionBehaviorName(ActionBehavior behavior)
{
    switch (behavior) {
    case ActionBehavior::Hold:
        return "Hold";
    case ActionBehavior::Toggle:
        return "Toggle";
    case ActionBehavior::Press:
    default:
        return "Press";
    }
}

inline const char *logicalActionId(LogicalAction action)
{
    switch (action) {
    case LogicalAction::MouseLeft: return "mouse.left";
    case LogicalAction::MouseRight: return "mouse.right";
    case LogicalAction::ScrollUp: return "mouse.scroll_up";
    case LogicalAction::ScrollDown: return "mouse.scroll_down";
    case LogicalAction::CursorFreeze: return "control.cursor.freeze";
    case LogicalAction::KeySpace: return "key.space";
    case LogicalAction::KeyEnter: return "key.enter";
    case LogicalAction::KeyEscape: return "key.escape";
    case LogicalAction::KeyTab: return "key.tab";
    case LogicalAction::KeyLeft: return "key.left";
    case LogicalAction::KeyRight: return "key.right";
    case LogicalAction::KeyUp: return "key.up";
    case LogicalAction::KeyDown: return "key.down";
    case LogicalAction::KeyCtrl: return "key.ctrl";
    case LogicalAction::KeyShift: return "key.shift";
    case LogicalAction::KeyAlt: return "key.alt";
    }
    return "mouse.left";
}

} // namespace vc
