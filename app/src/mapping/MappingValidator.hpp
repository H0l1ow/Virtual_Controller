#pragma once

#include "mapping/MappingTypes.hpp"

#include <cstddef>
#include <string>
#include <unordered_set>
#include <vector>

namespace vc {

// Pure domain validation: never edits profiles, never arms Windows input.
// Messages are represented by stable codes; QML translates their descriptions.
struct MappingIssue {
    enum class Severity { Warning, Error };
    std::string code;
    Severity severity{Severity::Warning};
    int first{-1};
    int second{-1};
};

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
            if (!other.enabled || other.hand != rule.hand)
                continue;
            const int otherIndex = static_cast<int>(j);

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
                // A single hand cannot hold two different mutually exclusive
                // static gestures at the same time, so dragging will not work.
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
