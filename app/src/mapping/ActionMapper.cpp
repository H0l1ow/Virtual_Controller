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
    case LogicalAction::CursorMoveEnable:
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
    ResolvedTriggerFrame resolved;
    resolved.timestampUs = events.timestampUs;
    resolved.hasTrackingStatus = events.hasTrackingStatus;
    resolved.tracked = events.tracked;
    resolved.trackIds = events.trackIds;
    for (std::size_t i = 0; i < kHandCount; ++i) {
        const auto &event = events.hands[i];
        if (event.phase != GestureEventPhase::None) {
            resolved.triggers.push_back({TriggerKind::Single, event.phase,
                i == handIndex(HandSide::Left) ? HandSide::Left : HandSide::Right,
                event.gesture, {}, event.trackId});
        }
    }
    return applyResolved(resolved, std::move(baseState));
}

ControllerState ActionMapper::applyResolved(
    const ResolvedTriggerFrame &events,
    ControllerState baseState)
{
    // Safety first: invalidate latched inputs before any new event on this frame.
    if (events.hasTrackingStatus) {
        for (std::size_t i = 0; i < kHandCount; ++i) {
            const HandSide side = i == handIndex(HandSide::Left)
                ? HandSide::Left : HandSide::Right;
            const auto id = events.tracked[i] ? events.trackIds[i] : 0;
            if (id == 0 || (trackIds_[i] != 0 && trackIds_[i] != id))
                neutralizeHand(side);
            trackIds_[i] = id;
        }
        for (auto it = comboTrackIds_.begin(); it != comboTrackIds_.end();) {
            const auto &ids = it->second;
            if (ids[0] != events.trackIds[0] || ids[1] != events.trackIds[1]
                || !events.tracked[0] || !events.tracked[1]) {
                held_.erase(it->first);
                toggled_.erase(it->first);
                it = comboTrackIds_.erase(it);
            } else {
                ++it;
            }
        }
    }

    for (const auto &event : events.triggers) {
        for (const auto &rule : mappings_) {
            if (!rule.enabled || rule.triggerKind != event.kind)
                continue;
            if (event.kind == TriggerKind::TwoHandCombination) {
                if (rule.id != event.combinationId)
                    continue;
                if (event.phase == GestureEventPhase::Press && events.hasTrackingStatus)
                    comboTrackIds_[rule.id] = events.trackIds;
            } else {
                if (!matches(rule, event.hand,
                             {event.gesture, event.phase, 0.0F, event.trackId}))
                    continue;
                if (rule.anyHand) {
                    const auto owner = anyOwners_.find(rule.id);
                    if (event.phase == GestureEventPhase::Press) {
                        if (owner != anyOwners_.end())
                            continue; // exactly one owner and one PRESS
                        anyOwners_.insert_or_assign(rule.id, event.hand);
                    } else {
                        if (owner == anyOwners_.end() || owner->second != event.hand)
                            continue;
                        if (event.phase == GestureEventPhase::Release)
                            anyOwners_.erase(owner);
                    }
                }
            }
            switch (rule.behavior) {
            case ActionBehavior::Press:
                if (event.phase == GestureEventPhase::Press)
                    applyAction(baseState, rule.action);
                break;
            case ActionBehavior::Hold:
                if (event.phase == GestureEventPhase::Press)
                    held_.insert(rule.id);
                else if (event.phase == GestureEventPhase::Release)
                    held_.erase(rule.id);
                break;
            case ActionBehavior::Toggle:
                if (event.phase == GestureEventPhase::Press) {
                    if (toggled_.contains(rule.id))
                        toggled_.erase(rule.id);
                    else
                        toggled_.insert(rule.id);
                }
                break;
            }
        }
    }

    applyPersistentStates(baseState);

    // Keep the original fail-closed pointer movement gate invariant.
    baseState.cursorMovementLocked = !cursorMoveGateActive();
    if (baseState.cursorFrozen || baseState.cursorMovementLocked) {
        baseState.mouseX = 0.0F;
        baseState.mouseY = 0.0F;
    }
    return baseState;
}

void ActionMapper::reset()
{
    held_.clear();
    toggled_.clear();
    anyOwners_.clear();
    comboTrackIds_.clear();
    trackIds_ = {};
}

void ActionMapper::neutralizeHand(HandSide hand)
{
    // Toggle is persistent only while the originating hand stays tracked.
    for (const auto &rule : mappings_) {
        if (rule.triggerKind == TriggerKind::TwoHandCombination) {
            held_.erase(rule.id);
            toggled_.erase(rule.id);
            comboTrackIds_.erase(rule.id);
        } else if (rule.anyHand) {
            const auto owner = anyOwners_.find(rule.id);
            if (owner != anyOwners_.end() && owner->second == hand) {
                held_.erase(rule.id);
                toggled_.erase(rule.id);
                anyOwners_.erase(owner);
            }
        } else if (rule.hand == hand) {
            held_.erase(rule.id);
            toggled_.erase(rule.id);
        }
    }
}

bool ActionMapper::matches(
    const MappingRule &rule,
    HandSide hand,
    const GestureEvent &event)
{
    return rule.triggerKind == TriggerKind::Single
        && (rule.anyHand || rule.hand == hand)
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
    case LogicalAction::CursorMoveEnable:
        // Gate activation is derived from held_/toggled_ mapping state after
        // all events are processed. It intentionally emits no OS action.
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


bool ActionMapper::cursorMoveGateActive() const
{
    return std::any_of(
        mappings_.begin(),
        mappings_.end(),
        [this](const MappingRule &rule) {
            if (!rule.enabled
                || rule.action != LogicalAction::CursorMoveEnable) {
                return false;
            }

            return held_.contains(rule.id)
                || toggled_.contains(rule.id);
        });
}

} // namespace vc
