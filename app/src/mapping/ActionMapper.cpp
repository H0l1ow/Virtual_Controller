#include "mapping/ActionMapper.hpp"

#include <algorithm>
#include <utility>

namespace vc {

namespace {

KeyCode logicalActionKey(LogicalAction action)
{
    switch (action) {
    case LogicalAction::KeySpace: return KeyCode::Space;
    case LogicalAction::KeyEnter: return KeyCode::Enter;
    case LogicalAction::KeyEscape: return KeyCode::Escape;
    case LogicalAction::KeyTab: return KeyCode::Tab;
    case LogicalAction::KeyLeft: return KeyCode::Left;
    case LogicalAction::KeyRight: return KeyCode::Right;
    case LogicalAction::KeyUp: return KeyCode::Up;
    case LogicalAction::KeyDown: return KeyCode::Down;
    case LogicalAction::KeyCtrl: return KeyCode::Ctrl;
    case LogicalAction::KeyShift: return KeyCode::Shift;
    case LogicalAction::KeyAlt: return KeyCode::Alt;
    case LogicalAction::MouseLeft:
    case LogicalAction::MouseRight:
    case LogicalAction::ScrollUp:
    case LogicalAction::ScrollDown:
    case LogicalAction::CursorFreeze:
        break;
    }
    return KeyCode::Space;
}

bool isKeyboardAction(LogicalAction action)
{
    return action >= LogicalAction::KeySpace;
}

} // namespace

void ActionMapper::setMappings(
    std::vector<MappingRule> mappings)
{
    mappings_ = std::move(mappings);
    reset();
}

ControllerState ActionMapper::apply(
    const GestureEventFrame &events,
    ControllerState baseState)
{
    for (std::size_t handIndexValue = 0;
         handIndexValue < kHandCount;
         ++handIndexValue) {
        const HandSide hand = handIndexValue == handIndex(HandSide::Left)
            ? HandSide::Left
            : HandSide::Right;
        const auto &event = events.hands[handIndexValue];

        for (const auto &rule : mappings_) {
            if (!rule.enabled || !matches(rule, hand, event)) {
                continue;
            }

            switch (rule.behavior) {
            case ActionBehavior::Press:
                if (event.phase == GestureEventPhase::Press) {
                    applyAction(baseState, rule.action);
                }
                break;

            case ActionBehavior::Hold:
                if (event.phase == GestureEventPhase::Press) {
                    held_.insert(rule.id);
                }
                else if (event.phase == GestureEventPhase::Release) {
                    held_.erase(rule.id);
                }
                break;

            case ActionBehavior::Toggle:
                if (event.phase == GestureEventPhase::Press) {
                    if (toggled_.contains(rule.id)) {
                        toggled_.erase(rule.id);
                    }
                    else {
                        toggled_.insert(rule.id);
                    }
                }
                break;
            }
        }
    }

    applyPersistentStates(baseState);

    // Freeze is deliberately applied after ContinuousControlInterpreter has
    // processed the current hand position. The interpreter therefore keeps
    // advancing its reference position while the visible/system cursor stays
    // still, preventing a jump when the freeze gesture is released.
    if (baseState.cursorFrozen) {
        baseState.mouseX = 0.0F;
        baseState.mouseY = 0.0F;
    }

    return baseState;
}

void ActionMapper::reset()
{
    held_.clear();
    toggled_.clear();
}

bool ActionMapper::matches(
    const MappingRule &rule,
    HandSide hand,
    const GestureEvent &event)
{
    return rule.hand == hand
        && rule.gesture == event.gesture
        && event.phase != GestureEventPhase::None;
}

void ActionMapper::applyAction(
    ControllerState &state,
    LogicalAction action)
{
    switch (action) {
    case LogicalAction::MouseLeft:
        state.mouseLeft = true;
        return;
    case LogicalAction::MouseRight:
        state.mouseRight = true;
        return;
    case LogicalAction::ScrollUp:
        state.wheel += 1.0F;
        return;
    case LogicalAction::ScrollDown:
        state.wheel -= 1.0F;
        return;
    case LogicalAction::CursorFreeze:
        state.cursorFrozen = true;
        return;
    default:
        if (isKeyboardAction(action)) {
            state.keys |= keyMask(logicalActionKey(action));
        }
        return;
    }
}

void ActionMapper::applyPersistentStates(
    ControllerState &state) const
{
    for (const auto &rule : mappings_) {
        if (!rule.enabled) {
            continue;
        }

        if (held_.contains(rule.id)
            || toggled_.contains(rule.id)) {
            applyAction(state, rule.action);
        }
    }
}

} // namespace vc
