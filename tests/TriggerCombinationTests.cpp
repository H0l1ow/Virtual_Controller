#include "gestures/GestureRegistry.hpp"
#include "gestures/TriggerCombinationEngine.hpp"
#include "mapping/ActionMapper.hpp"
#include "mapping/MappingValidator.hpp"

#include <cassert>
#include <iostream>

using namespace vc;

namespace {

MappingRule single(const char *id, HandSide side, GestureClass gesture,
                   LogicalAction action, ActionBehavior behavior = ActionBehavior::Press)
{
    return {id, side, gesture, action, behavior, true};
}

MappingRule pair(const char *id, GestureClass left, GestureClass right,
                 LogicalAction action, ActionBehavior behavior = ActionBehavior::Press,
                 int sync = 250, int hold = 150)
{
    MappingRule rule{id, HandSide::Left, left, action, behavior, true};
    rule.triggerKind = TriggerKind::TwoHandCombination;
    rule.rightGesture = right;
    rule.synchronizationMs = sync;
    rule.overlapHoldMs = hold;
    return rule;
}

GestureEventFrame events(
    std::int64_t time, GestureClass left, GestureEventPhase lPhase,
    GestureClass right = GestureClass::None,
    GestureEventPhase rPhase = GestureEventPhase::None,
    bool lTracked = true, bool rTracked = true,
    std::uint64_t lTrack = 1, std::uint64_t rTrack = 2)
{
    GestureEventFrame result;
    result.timestampUs = time * 1000;
    result.hasTrackingStatus = true;
    result.tracked = {lTracked, rTracked};
    result.trackIds = {lTracked ? lTrack : 0, rTracked ? rTrack : 0};
    result.hands[0] = {left, lPhase, 0.9F, lTrack};
    result.hands[1] = {right, rPhase, 0.9F, rTrack};
    return result;
}

bool key(const ControllerState &state, KeyCode code)
{
    return (state.keys & keyMask(code)) != 0;
}

void verifyPairPreemptsAllSingletons()
{
    const std::vector<MappingRule> rules{
        single("left", HandSide::Left, GestureClass::Fist, LogicalAction::KeySpace),
        single("right", HandSide::Right, GestureClass::Fist, LogicalAction::KeyEnter),
        pair("both", GestureClass::Fist, GestureClass::Fist, LogicalAction::KeyTab)
    };
    TriggerCombinationEngine engine;
    ActionMapper mapper;
    engine.setMappings(rules);
    mapper.setMappings(rules);
    auto output = mapper.applyResolved(engine.update(events(10, GestureClass::Fist, GestureEventPhase::Press,
                                                       GestureClass::Fist, GestureEventPhase::Press)), {});
    assert(!key(output, KeyCode::Space) && !key(output, KeyCode::Enter));
    assert(!key(output, KeyCode::Tab));
    output = mapper.applyResolved(engine.update(events(159, GestureClass::Fist, GestureEventPhase::Hold,
                                                GestureClass::Fist, GestureEventPhase::Hold)), {});
    assert(!key(output, KeyCode::Tab));
    output = mapper.applyResolved(engine.update(events(160, GestureClass::Fist, GestureEventPhase::Hold,
                                                GestureClass::Fist, GestureEventPhase::Hold)), {});
    assert(key(output, KeyCode::Tab) && !key(output, KeyCode::Space)
           && !key(output, KeyCode::Enter));
    output = mapper.applyResolved(engine.update(events(200, GestureClass::Fist, GestureEventPhase::Hold,
                                                GestureClass::Fist, GestureEventPhase::Hold)), {});
    assert(!key(output, KeyCode::Tab) && !key(output, KeyCode::Space));
    output = mapper.applyResolved(engine.update(events(210, GestureClass::Fist, GestureEventPhase::Release,
                                                GestureClass::Fist, GestureEventPhase::Hold)), {});
    assert(!key(output, KeyCode::Space) && !key(output, KeyCode::Enter));
    output = mapper.applyResolved(engine.update(events(500, GestureClass::None, GestureEventPhase::None,
                                                GestureClass::Fist, GestureEventPhase::Hold)), {});
    assert(!key(output, KeyCode::Enter)); // no singleton activation after pair release
}

void verifyUnmappedAndDeferredSingles()
{
    std::vector<MappingRule> rules{
        single("left", HandSide::Left, GestureClass::Fist, LogicalAction::KeySpace),
        pair("both", GestureClass::Fist, GestureClass::Fist, LogicalAction::KeyTab)
    };
    TriggerCombinationEngine engine;
    ActionMapper mapper;
    engine.setMappings(rules);
    mapper.setMappings(rules);
    auto output = mapper.applyResolved(engine.update(events(10, GestureClass::Fist,
                                      GestureEventPhase::Press)), {});
    assert(!key(output, KeyCode::Space));
    output = mapper.applyResolved(engine.update(events(259, GestureClass::Fist,
                                      GestureEventPhase::Hold)), {});
    assert(!key(output, KeyCode::Space));
    output = mapper.applyResolved(engine.update(events(260, GestureClass::Fist,
                                      GestureEventPhase::Hold)), {});
    assert(key(output, KeyCode::Space));
    output = mapper.applyResolved(engine.update(events(450, GestureClass::Fist,
                                      GestureEventPhase::Hold,
                                      GestureClass::Fist, GestureEventPhase::Press)), {});
    assert(!key(output, KeyCode::Tab)); // outside 250 ms, no retroactive pair
    engine.setMappings({rules[0]});
    mapper.setMappings({rules[0]});
    output = mapper.applyResolved(engine.update(events(600, GestureClass::Fist,
                                      GestureEventPhase::Press)), {});
    assert(key(output, KeyCode::Space)); // no configured pair = no extra delay
}

void verifyOrderedPairsAndTiming()
{
    std::vector<MappingRule> rules{
        single("one", HandSide::Left, GestureClass::Fist, LogicalAction::KeySpace),
        pair("LR", GestureClass::Fist, GestureClass::Point, LogicalAction::KeyTab),
        pair("RL", GestureClass::Point, GestureClass::Fist, LogicalAction::KeyEnter),
    };
    TriggerCombinationEngine engine;
    ActionMapper mapper;
    engine.setMappings(rules);
    mapper.setMappings(rules);
    mapper.applyResolved(engine.update(events(10, GestureClass::Fist, GestureEventPhase::Press)), {});
    mapper.applyResolved(engine.update(events(260, GestureClass::Fist, GestureEventPhase::Hold,
                                       GestureClass::Point, GestureEventPhase::Press)), {});
    auto output = mapper.applyResolved(engine.update(events(409, GestureClass::Fist, GestureEventPhase::Hold,
                                                    GestureClass::Point, GestureEventPhase::Hold)), {});
    assert(!key(output, KeyCode::Tab) && !key(output, KeyCode::Space));
    output = mapper.applyResolved(engine.update(events(410, GestureClass::Fist, GestureEventPhase::Hold,
                                                GestureClass::Point, GestureEventPhase::Hold)), {});
    assert(key(output, KeyCode::Tab) && !key(output, KeyCode::Space));
    engine.reset();
    mapper.reset();
    mapper.applyResolved(engine.update(events(500, GestureClass::Point, GestureEventPhase::Press,
                                       GestureClass::Fist, GestureEventPhase::Press)), {});
    output = mapper.applyResolved(engine.update(events(650, GestureClass::Point, GestureEventPhase::Hold,
                                                GestureClass::Fist, GestureEventPhase::Hold)), {});
    assert(key(output, KeyCode::Enter) && !key(output, KeyCode::Tab));
}

void verifyImmediateLostHandRelease()
{
    std::vector<MappingRule> rules{
        pair("mouse", GestureClass::Fist, GestureClass::Pinch,
             LogicalAction::MouseLeft, ActionBehavior::Hold, 250, 50)
    };
    TriggerCombinationEngine engine;
    ActionMapper mapper;
    engine.setMappings(rules);
    mapper.setMappings(rules);
    mapper.applyResolved(engine.update(events(10, GestureClass::Fist, GestureEventPhase::Press,
                                       GestureClass::Pinch, GestureEventPhase::Press)), {});
    auto output = mapper.applyResolved(engine.update(events(60, GestureClass::Fist, GestureEventPhase::Hold,
                                                    GestureClass::Pinch, GestureEventPhase::Hold)), {});
    assert(output.mouseLeft);
    output = mapper.applyResolved(engine.update(events(61, GestureClass::Fist, GestureEventPhase::Release,
                                                GestureClass::Pinch, GestureEventPhase::Hold,
                                                false, true)), {});
    assert(!output.mouseLeft);
    output = mapper.applyResolved(engine.update(events(62, GestureClass::None, GestureEventPhase::None,
                                                GestureClass::Pinch, GestureEventPhase::Hold,
                                                false, true)), {});
    assert(!output.mouseLeft);
}

void verifyAnyHandOwnership()
{
    auto rule = single("any", HandSide::Right, GestureClass::Fist,
                       LogicalAction::KeyCtrl, ActionBehavior::Toggle);
    rule.anyHand = true;
    TriggerCombinationEngine engine;
    ActionMapper mapper;
    engine.setMappings({rule});
    mapper.setMappings({rule});
    auto output = mapper.applyResolved(engine.update(events(10, GestureClass::Fist, GestureEventPhase::Press,
                                                       GestureClass::Fist, GestureEventPhase::Press)), {});
    assert(key(output, KeyCode::Ctrl)); // not toggled twice in same frame
    output = mapper.applyResolved(engine.update(events(11, GestureClass::Fist, GestureEventPhase::Release,
                                                GestureClass::Fist, GestureEventPhase::Hold,
                                                false, true)), {});
    assert(!key(output, KeyCode::Ctrl)); // owner lost; other hand cannot inherit
}

void verifyShortPairFailureFallsBack()
{
    std::vector<MappingRule> rules{
        single("left", HandSide::Left, GestureClass::Fist, LogicalAction::KeySpace),
        single("right", HandSide::Right, GestureClass::Point, LogicalAction::KeyEnter),
        pair("both", GestureClass::Fist, GestureClass::Point, LogicalAction::KeyTab)
    };
    TriggerCombinationEngine engine;
    ActionMapper mapper;
    engine.setMappings(rules);
    mapper.setMappings(rules);
    mapper.applyResolved(engine.update(events(10, GestureClass::Fist, GestureEventPhase::Press)), {});
    mapper.applyResolved(engine.update(events(100, GestureClass::Fist, GestureEventPhase::Hold,
                                               GestureClass::Point, GestureEventPhase::Press)), {});
    auto output = mapper.applyResolved(engine.update(events(120, GestureClass::Fist, GestureEventPhase::Hold,
                                                        GestureClass::Point, GestureEventPhase::Release)), {});
    assert(key(output, KeyCode::Enter) && !key(output, KeyCode::Tab));
    output = mapper.applyResolved(engine.update(events(260, GestureClass::Fist, GestureEventPhase::Hold)), {});
    assert(key(output, KeyCode::Space) && !key(output, KeyCode::Tab));
    output = mapper.applyResolved(engine.update(events(300, GestureClass::Fist, GestureEventPhase::Release)), {});
    assert(!key(output, KeyCode::Space));
}

void verifyPairHoldAndClockReset()
{
    std::vector<MappingRule> rules{
        pair("hold", GestureClass::Fist, GestureClass::Fist,
             LogicalAction::KeyShift, ActionBehavior::Hold, 250, 25)
    };
    TriggerCombinationEngine engine;
    ActionMapper mapper;
    engine.setMappings(rules);
    mapper.setMappings(rules);
    mapper.applyResolved(engine.update(events(50, GestureClass::Fist, GestureEventPhase::Press,
                                               GestureClass::Fist, GestureEventPhase::Press)), {});
    auto output = mapper.applyResolved(engine.update(events(75, GestureClass::Fist, GestureEventPhase::Hold,
                                                        GestureClass::Fist, GestureEventPhase::Hold)), {});
    assert(key(output, KeyCode::Shift));
    output = mapper.applyResolved(engine.update(events(76, GestureClass::Fist, GestureEventPhase::Hold,
                                                       GestureClass::Fist, GestureEventPhase::Hold)), {});
    assert(key(output, KeyCode::Shift));
    output = mapper.applyResolved(engine.update(events(1, GestureClass::None, GestureEventPhase::None,
                                                       GestureClass::None, GestureEventPhase::None)), {});
    assert(!key(output, KeyCode::Shift)); // discontinuity must RELEASE
}

void verifyPairToggleDiesWithEitherTrack()
{
    std::vector<MappingRule> rules{
        pair("toggle", GestureClass::Fist, GestureClass::Pinch,
             LogicalAction::KeyCtrl, ActionBehavior::Toggle, 250, 0)
    };
    TriggerCombinationEngine engine;
    ActionMapper mapper;
    engine.setMappings(rules);
    mapper.setMappings(rules);
    auto output = mapper.applyResolved(engine.update(events(10, GestureClass::Fist, GestureEventPhase::Press,
                                                         GestureClass::Pinch, GestureEventPhase::Press)), {});
    assert(key(output, KeyCode::Ctrl));
    output = mapper.applyResolved(engine.update(events(30, GestureClass::Fist, GestureEventPhase::Hold,
                                                        GestureClass::Pinch, GestureEventPhase::Hold)), {});
    assert(key(output, KeyCode::Ctrl));
    output = mapper.applyResolved(engine.update(events(31, GestureClass::Fist, GestureEventPhase::Hold,
                                                        GestureClass::Pinch, GestureEventPhase::Release,
                                                        true, false)), {});
    assert(!key(output, KeyCode::Ctrl)); // even a toggled pair is fail-closed
}

void verifyExperimentalPosePair()
{
    // M6.4.2: two experimental static poses enter exactly the same
    // precedence/safety path as the original four recognized classes.
    const std::vector<MappingRule> rules{
        single("up", HandSide::Left, GestureClass::ThumbsUp, LogicalAction::KeySpace),
        single("v", HandSide::Right, GestureClass::Victory, LogicalAction::KeyEnter),
        pair("new_pair", GestureClass::ThumbsUp, GestureClass::Victory,
             LogicalAction::KeyTab, ActionBehavior::Hold, 250, 100)
    };
    TriggerCombinationEngine engine;
    ActionMapper mapper;
    engine.setMappings(rules);
    mapper.setMappings(rules);
    auto output = mapper.applyResolved(engine.update(events(
        10, GestureClass::ThumbsUp, GestureEventPhase::Press,
        GestureClass::Victory, GestureEventPhase::Press)), {});
    assert(!key(output, KeyCode::Space) && !key(output, KeyCode::Enter));
    output = mapper.applyResolved(engine.update(events(
        110, GestureClass::ThumbsUp, GestureEventPhase::Hold,
        GestureClass::Victory, GestureEventPhase::Hold)), {});
    assert(key(output, KeyCode::Tab));
    assert(!key(output, KeyCode::Space) && !key(output, KeyCode::Enter));
    output = mapper.applyResolved(engine.update(events(
        112, GestureClass::ThumbsUp, GestureEventPhase::Hold,
        GestureClass::Victory, GestureEventPhase::Release, true, false)), {});
    assert(!key(output, KeyCode::Tab) && !key(output, KeyCode::Space));
}

void verifyRegistryAndValidation()
{
    assert(kGestureRegistry.size() == 26);
    assert(isAvailableGestureId("FIST"));
    assert(isAvailableGestureId("THUMBS_UP"));
    assert(!isAvailableGestureId("SWIPE_LEFT"));
    assert(!isRecognizedGesture(GestureClass::None));
    MappingProfile profile;
    profile.mappings.push_back(pair("one", GestureClass::Fist, GestureClass::Point,
                                    LogicalAction::KeyTab));
    profile.mappings.push_back(pair("two", GestureClass::Fist, GestureClass::Point,
                                    LogicalAction::KeySpace));
    const auto issues = validateMappings(profile);
    bool duplicate = false;
    for (const auto &issue : issues) {
        if (issue.code == "duplicate_combo"
            && issue.severity == MappingIssue::Severity::Error)
            duplicate = true;
    }
    assert(duplicate && hasBlockingMappingIssues(profile));
}

} // namespace

int main()
{
    verifyRegistryAndValidation();
    verifyExperimentalPosePair();
    verifyShortPairFailureFallsBack();
    verifyPairHoldAndClockReset();
    verifyPairToggleDiesWithEitherTrack();
    verifyPairPreemptsAllSingletons();
    verifyUnmappedAndDeferredSingles();
    verifyOrderedPairsAndTiming();
    verifyImmediateLostHandRelease();
    verifyAnyHandOwnership();
    std::cout << "M6.4.1 combination tests passed\n";
}
