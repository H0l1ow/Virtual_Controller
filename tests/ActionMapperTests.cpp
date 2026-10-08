#include "mapping/ActionMapper.hpp"

#include <cassert>

namespace {

vc::GestureEventFrame eventFrame(
    vc::HandSide hand,
    vc::GestureClass gesture,
    vc::GestureEventPhase phase)
{
    vc::GestureEventFrame frame;
    auto &event = frame.hands[vc::handIndex(hand)];
    event.gesture = gesture;
    event.phase = phase;
    event.confidence = 0.95F;
    event.trackId = 1;
    return frame;
}

} // namespace

int main()
{
    vc::ActionMapper mapper;
    mapper.setMappings({
        {"pinch_left", vc::HandSide::Right, vc::GestureClass::Pinch,
            vc::LogicalAction::MouseLeft, vc::ActionBehavior::Hold, true},
        {"fist_space", vc::HandSide::Right, vc::GestureClass::Fist,
            vc::LogicalAction::KeySpace, vc::ActionBehavior::Press, true},
        {"point_scroll", vc::HandSide::Left, vc::GestureClass::Point,
            vc::LogicalAction::ScrollUp, vc::ActionBehavior::Press, true},
        {"open_ctrl_toggle", vc::HandSide::Left, vc::GestureClass::OpenHand,
            vc::LogicalAction::KeyCtrl, vc::ActionBehavior::Toggle, true},
        {"fist_freeze", vc::HandSide::Right, vc::GestureClass::Fist,
            vc::LogicalAction::CursorFreeze, vc::ActionBehavior::Hold, true}
    });

    auto state = mapper.apply(
        eventFrame(vc::HandSide::Right, vc::GestureClass::Pinch, vc::GestureEventPhase::Press),
        {});
    assert(state.mouseLeft);

    state = mapper.apply(
        eventFrame(vc::HandSide::Right, vc::GestureClass::Pinch, vc::GestureEventPhase::Hold),
        {});
    assert(state.mouseLeft);

    state = mapper.apply(
        eventFrame(vc::HandSide::Right, vc::GestureClass::Pinch, vc::GestureEventPhase::Release),
        {});
    assert(!state.mouseLeft);

    vc::ControllerState moving;
    moving.mouseX = 42.0F;
    moving.mouseY = -18.0F;
    state = mapper.apply(
        eventFrame(vc::HandSide::Right, vc::GestureClass::Fist, vc::GestureEventPhase::Press),
        moving);
    assert((state.keys & vc::keyMask(vc::KeyCode::Space)) != 0);
    assert(state.cursorFrozen);
    assert(state.mouseX == 0.0F);
    assert(state.mouseY == 0.0F);

    // Press actions are one-shot logical frames, while Hold freeze persists.
    moving = {};
    moving.mouseX = 12.0F;
    state = mapper.apply({}, moving);
    assert((state.keys & vc::keyMask(vc::KeyCode::Space)) == 0);
    assert(state.cursorFrozen);
    assert(state.mouseX == 0.0F);

    state = mapper.apply(
        eventFrame(vc::HandSide::Right, vc::GestureClass::Fist, vc::GestureEventPhase::Release),
        moving);
    assert(!state.cursorFrozen);
    assert(state.cursorMovementLocked);
    assert(state.mouseX == 0.0F);

    state = mapper.apply(
        eventFrame(vc::HandSide::Left, vc::GestureClass::Point, vc::GestureEventPhase::Press),
        {});
    assert(state.wheel == 1.0F);

    state = mapper.apply(
        eventFrame(vc::HandSide::Left, vc::GestureClass::OpenHand, vc::GestureEventPhase::Press),
        {});
    assert((state.keys & vc::keyMask(vc::KeyCode::Ctrl)) != 0);
    state = mapper.apply({}, {});
    assert((state.keys & vc::keyMask(vc::KeyCode::Ctrl)) != 0);

    state = mapper.apply(
        eventFrame(vc::HandSide::Left, vc::GestureClass::OpenHand, vc::GestureEventPhase::Press),
        {});
    assert((state.keys & vc::keyMask(vc::KeyCode::Ctrl)) == 0);

    mapper.reset();
    assert(mapper.apply({}, {}).neutral());

    // Cursor movement is denied by default. Arming output alone does not
    // permit movement; an Enable cursor movement mapping must be active.
    vc::ActionMapper unrestricted;
    vc::ControllerState unrestrictedMove;
    unrestrictedMove.mouseX = 15.0F;
    unrestrictedMove.mouseY = -7.0F;
    state = unrestricted.apply({}, unrestrictedMove);
    assert(state.cursorMovementLocked);
    assert(state.mouseX == 0.0F);
    assert(state.mouseY == 0.0F);

    // Hold gate: movement is suppressed until the mapped gesture is active,
    // allowed for the duration of the gesture, then suppressed again.
    vc::ActionMapper holdGate;
    holdGate.setMappings({
        {"point_move_gate", vc::HandSide::Right, vc::GestureClass::Point,
            vc::LogicalAction::CursorMoveEnable, vc::ActionBehavior::Hold, true}
    });

    vc::ControllerState gatedMove;
    gatedMove.mouseX = 21.0F;
    gatedMove.mouseY = 9.0F;
    state = holdGate.apply({}, gatedMove);
    assert(state.cursorMovementLocked);
    assert(state.mouseX == 0.0F);
    assert(state.mouseY == 0.0F);

    state = holdGate.apply(
        eventFrame(vc::HandSide::Right, vc::GestureClass::Point, vc::GestureEventPhase::Press),
        gatedMove);
    assert(!state.cursorMovementLocked);
    assert(state.mouseX == 21.0F);
    assert(state.mouseY == 9.0F);

    state = holdGate.apply(
        eventFrame(vc::HandSide::Right, vc::GestureClass::Point, vc::GestureEventPhase::Hold),
        gatedMove);
    assert(!state.cursorMovementLocked);
    assert(state.mouseX == 21.0F);

    state = holdGate.apply(
        eventFrame(vc::HandSide::Right, vc::GestureClass::Point, vc::GestureEventPhase::Release),
        gatedMove);
    assert(state.cursorMovementLocked);
    assert(state.mouseX == 0.0F);
    assert(state.mouseY == 0.0F);

    // Toggle gate remains active between frames until the next PRESS.
    vc::ActionMapper toggleGate;
    toggleGate.setMappings({
        {"fist_move_toggle", vc::HandSide::Left, vc::GestureClass::Fist,
            vc::LogicalAction::CursorMoveEnable, vc::ActionBehavior::Toggle, true}
    });
    state = toggleGate.apply({}, gatedMove);
    assert(state.cursorMovementLocked);

    state = toggleGate.apply(
        eventFrame(vc::HandSide::Left, vc::GestureClass::Fist, vc::GestureEventPhase::Press),
        gatedMove);
    assert(!state.cursorMovementLocked);
    state = toggleGate.apply({}, gatedMove);
    assert(!state.cursorMovementLocked);

    state = toggleGate.apply(
        eventFrame(vc::HandSide::Left, vc::GestureClass::Fist, vc::GestureEventPhase::Press),
        gatedMove);
    assert(state.cursorMovementLocked);
    assert(state.mouseX == 0.0F);

    // Freeze always wins even while a movement gate is active.
    vc::ActionMapper freezeAndGate;
    freezeAndGate.setMappings({
        {"point_move_gate", vc::HandSide::Right, vc::GestureClass::Point,
            vc::LogicalAction::CursorMoveEnable, vc::ActionBehavior::Hold, true},
        {"fist_freeze_again", vc::HandSide::Left, vc::GestureClass::Fist,
            vc::LogicalAction::CursorFreeze, vc::ActionBehavior::Hold, true}
    });
    state = freezeAndGate.apply(
        eventFrame(vc::HandSide::Right, vc::GestureClass::Point, vc::GestureEventPhase::Press),
        gatedMove);
    assert(!state.cursorMovementLocked);
    assert(state.mouseX == 21.0F);

    state = freezeAndGate.apply(
        eventFrame(vc::HandSide::Left, vc::GestureClass::Fist, vc::GestureEventPhase::Press),
        gatedMove);
    assert(!state.cursorMovementLocked);
    assert(state.cursorFrozen);
    assert(state.mouseX == 0.0F);
    assert(state.mouseY == 0.0F);

    // A disabled movement-gate mapping does not unlock movement.
    vc::ActionMapper disabledGate;
    disabledGate.setMappings({
        {"disabled_move_gate", vc::HandSide::Right, vc::GestureClass::Point,
            vc::LogicalAction::CursorMoveEnable, vc::ActionBehavior::Hold, false}
    });
    state = disabledGate.apply({}, gatedMove);
    assert(state.cursorMovementLocked);
    assert(state.mouseX == 0.0F);

    return 0;
}
