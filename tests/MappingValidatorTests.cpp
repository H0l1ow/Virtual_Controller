#include "mapping/MappingValidator.hpp"

#include <algorithm>
#include <cassert>
#include <string>

using namespace vc;

namespace {
MappingRule rule(const char *id, HandSide hand, GestureClass gesture,
                 LogicalAction action, ActionBehavior behavior, bool enabled = true)
{
    MappingRule value;
    value.id = id;
    value.hand = hand;
    value.gesture = gesture;
    value.action = action;
    value.behavior = behavior;
    value.enabled = enabled;
    return value;
}

bool contains(const std::vector<MappingIssue> &issues, const std::string &code)
{
    return std::any_of(issues.begin(), issues.end(), [&](const MappingIssue &issue) {
        return issue.code == code;
    });
}
} // namespace

int main()
{
    MappingProfile profile;
    profile.mappings = {
        rule("gate", HandSide::Right, GestureClass::OpenHand,
             LogicalAction::CursorMoveEnable, ActionBehavior::Hold),
        rule("click", HandSide::Right, GestureClass::Pinch,
             LogicalAction::MouseLeft, ActionBehavior::Hold),
        rule("other", HandSide::Left, GestureClass::Pinch,
             LogicalAction::MouseLeft, ActionBehavior::Hold),
    };
    assert(contains(validateMappings(profile), "drag_exclusive"));
    assert(!contains(validateMappings(profile), "no_cursor_gate"));
    assert(!hasBlockingMappingIssues(profile));

    profile.mappings[1].hand = HandSide::Left;
    assert(!contains(validateMappings(profile), "drag_exclusive"));

    profile.mappings[1].id = "gate";
    assert(contains(validateMappings(profile), "duplicate_id"));
    assert(hasBlockingMappingIssues(profile));

    profile.mappings[1].id = "click";
    profile.mappings[0].enabled = false;
    assert(contains(validateMappings(profile), "no_cursor_gate"));

    profile.mappings[0].enabled = true;
    profile.mappings[1].hand = HandSide::Right;
    profile.mappings[1].gesture = GestureClass::OpenHand;
    profile.mappings[1].action = LogicalAction::CursorMoveEnable;
    profile.mappings[1].behavior = ActionBehavior::Toggle;
    assert(contains(validateMappings(profile), "conflicting_behavior"));

    profile.mappings[1].behavior = ActionBehavior::Hold;
    assert(contains(validateMappings(profile), "duplicate_action"));

    profile.mappings[1].action = LogicalAction::CursorFreeze;
    assert(contains(validateMappings(profile), "cursor_gate_freeze"));

    profile.mappings[1].action = LogicalAction::ScrollDown;
    profile.mappings[1].behavior = ActionBehavior::Toggle;
    assert(contains(validateMappings(profile), "scroll_toggle"));
    assert(hasBlockingMappingIssues(profile));

    profile.mappings[1].enabled = false;
    assert(!contains(validateMappings(profile), "scroll_toggle"));
    return 0;
}
