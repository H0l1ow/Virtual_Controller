#pragma once

#include "gestures/GestureRegistry.hpp"
#include "mapping/MappingTypes.hpp"

#include <cstddef>
#include <string>
#include <unordered_set>
#include <vector>

namespace vc {

// Pure validation. Warnings describe deterministic precedence, not errors.
struct MappingIssue {
    enum class Severity { Warning, Error };
    std::string code;
    Severity severity{Severity::Warning};
    int first{-1};
    int second{-1};
};

inline bool singleConflictsWithPair(const MappingRule &single, const MappingRule &pair)
{
    return ((single.anyHand || single.hand == HandSide::Left)
            && single.gesture == pair.gesture)
        || ((single.anyHand || single.hand == HandSide::Right)
            && single.gesture == pair.rightGesture);
}

inline std::vector<MappingIssue> validateMappings(const MappingProfile &profile)
{
    std::vector<MappingIssue> issues;
    std::unordered_set<std::string> ids;
    bool hasCursorGate = false;
    for (std::size_t i = 0; i < profile.mappings.size(); ++i) {
        const auto &rule = profile.mappings[i];
        const int index = static_cast<int>(i);
        if (rule.id.empty() || !ids.insert(rule.id).second)
            issues.push_back({"duplicate_id", MappingIssue::Severity::Error, index, -1});
        if (!rule.enabled)
            continue;
        if (!isRecognizedGesture(rule.gesture)
            || (rule.triggerKind == TriggerKind::TwoHandCombination
                && !isRecognizedGesture(rule.rightGesture))) {
            // Legacy NONE singleton is inert, but keeping it lets old custom
            // profiles migrate losslessly. Any other unsupported ID is an error.
            const bool inertLegacy = rule.triggerKind == TriggerKind::Single
                && rule.gesture == GestureClass::None;
            issues.push_back({"unavailable_gesture", inertLegacy
                ? MappingIssue::Severity::Warning : MappingIssue::Severity::Error, index, -1});
        }
        if (rule.triggerKind == TriggerKind::TwoHandCombination
            && (rule.synchronizationMs < 0 || rule.synchronizationMs > 1500
                || rule.overlapHoldMs < 0 || rule.overlapHoldMs > 1500)) {
            issues.push_back({"invalid_combo_timing", MappingIssue::Severity::Error, index, -1});
        }
        if (rule.action == LogicalAction::CursorMoveEnable)
            hasCursorGate = true;
        if ((rule.action == LogicalAction::ScrollUp || rule.action == LogicalAction::ScrollDown)
            && rule.behavior == ActionBehavior::Toggle)
            issues.push_back({"scroll_toggle", MappingIssue::Severity::Error, index, -1});
        if (rule.action == LogicalAction::CursorMoveEnable
            && rule.behavior == ActionBehavior::Press)
            issues.push_back({"cursor_press", MappingIssue::Severity::Error, index, -1});

        for (std::size_t j = 0; j < i; ++j) {
            const auto &other = profile.mappings[j];
            if (!other.enabled)
                continue;
            const int otherIndex = static_cast<int>(j);
            if (rule.triggerKind != other.triggerKind) {
                const auto &pair = rule.triggerKind == TriggerKind::TwoHandCombination ? rule : other;
                const auto &single = rule.triggerKind == TriggerKind::Single ? rule : other;
                if (singleConflictsWithPair(single, pair))
                    issues.push_back({"combo_preempts_single", MappingIssue::Severity::Warning,
                                      otherIndex, index});
                continue;
            }
            if (rule.triggerKind == TriggerKind::TwoHandCombination) {
                if (rule.gesture == other.gesture && rule.rightGesture == other.rightGesture)
                    issues.push_back({"duplicate_combo", MappingIssue::Severity::Error,
                                      otherIndex, index});
                continue;
            }
            if (rule.hand != other.hand && !rule.anyHand && !other.anyHand)
                continue;
            if (other.gesture == rule.gesture) {
                if (other.action == rule.action) {
                    issues.push_back({other.behavior == rule.behavior
                                      ? "duplicate_action" : "conflicting_behavior",
                                      MappingIssue::Severity::Warning, otherIndex, index});
                } else if ((other.action == LogicalAction::CursorFreeze
                            && rule.action == LogicalAction::CursorMoveEnable)
                           || (other.action == LogicalAction::CursorMoveEnable
                               && rule.action == LogicalAction::CursorFreeze)) {
                    issues.push_back({"cursor_gate_freeze", MappingIssue::Severity::Warning,
                                      otherIndex, index});
                }
            } else {
                const auto cursorGateHold = [](const MappingRule &r) {
                    return r.action == LogicalAction::CursorMoveEnable
                        && r.behavior == ActionBehavior::Hold;
                };
                const auto mouseHold = [](const MappingRule &r) {
                    return (r.action == LogicalAction::MouseLeft
                            || r.action == LogicalAction::MouseRight)
                        && r.behavior == ActionBehavior::Hold;
                };
                if ((cursorGateHold(other) && mouseHold(rule))
                    || (cursorGateHold(rule) && mouseHold(other))) {
                    issues.push_back({"drag_exclusive", MappingIssue::Severity::Warning,
                                      otherIndex, index});
                }
            }
        }
    }
    if (!hasCursorGate)
        issues.push_back({"no_cursor_gate", MappingIssue::Severity::Warning, -1, -1});
    return issues;
}

inline bool hasBlockingMappingIssues(const MappingProfile &profile)
{
    for (const auto &issue : validateMappings(profile)) {
        if (issue.severity == MappingIssue::Severity::Error)
            return true;
    }
    return false;
}

} // namespace vc
