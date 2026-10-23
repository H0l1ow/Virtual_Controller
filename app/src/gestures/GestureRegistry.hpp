#pragma once

#include "gestures/GestureTypes.hpp"

#include <array>
#include <string_view>

namespace vc {

// The catalog is NOT the ONNX class list. Changing it must never change
// kGestureClassCount, model metadata, or the five-class inference contract.
enum class GestureCategory { Static, Dynamic, Temporal };
enum class GestureAvailability { Available, Experimental, Planned };

struct GestureDescriptor {
    std::string_view id;
    std::string_view label;
    GestureCategory category;
    GestureAvailability availability;
    GestureClass recognizedClass{GestureClass::None};
};

inline constexpr std::array<GestureDescriptor, 26> kGestureRegistry{{
    {"FIST", "Fist", GestureCategory::Static, GestureAvailability::Available, GestureClass::Fist},
    {"OPEN_HAND", "Open Hand", GestureCategory::Static, GestureAvailability::Available, GestureClass::OpenHand},
    {"POINT", "Point", GestureCategory::Static, GestureAvailability::Available, GestureClass::Point},
    {"PINCH", "Pinch", GestureCategory::Static, GestureAvailability::Available, GestureClass::Pinch},
    {"THUMBS_UP", "Thumbs Up", GestureCategory::Static, GestureAvailability::Experimental, GestureClass::ThumbsUp},
    {"THUMBS_DOWN", "Thumbs Down", GestureCategory::Static, GestureAvailability::Experimental, GestureClass::ThumbsDown},
    {"VICTORY", "Victory", GestureCategory::Static, GestureAvailability::Experimental, GestureClass::Victory},
    {"ROCK", "Rock", GestureCategory::Static, GestureAvailability::Experimental, GestureClass::Rock},
    {"THREE_FINGERS", "Three Fingers", GestureCategory::Static, GestureAvailability::Experimental, GestureClass::ThreeFingers},
    {"FOUR_FINGERS", "Four Fingers", GestureCategory::Static, GestureAvailability::Experimental, GestureClass::FourFingers},
    {"CALL_ME", "Call Me", GestureCategory::Static, GestureAvailability::Experimental, GestureClass::CallMe},
    {"OK", "OK", GestureCategory::Static, GestureAvailability::Experimental, GestureClass::Ok},
    {"SWIPE_LEFT", "Swipe Left", GestureCategory::Dynamic, GestureAvailability::Planned},
    {"SWIPE_RIGHT", "Swipe Right", GestureCategory::Dynamic, GestureAvailability::Planned},
    {"SWIPE_UP", "Swipe Up", GestureCategory::Dynamic, GestureAvailability::Planned},
    {"SWIPE_DOWN", "Swipe Down", GestureCategory::Dynamic, GestureAvailability::Planned},
    {"PUSH", "Push", GestureCategory::Dynamic, GestureAvailability::Planned},
    {"PULL", "Pull", GestureCategory::Dynamic, GestureAvailability::Planned},
    {"ROTATE_LEFT", "Rotate Left", GestureCategory::Dynamic, GestureAvailability::Planned},
    {"ROTATE_RIGHT", "Rotate Right", GestureCategory::Dynamic, GestureAvailability::Planned},
    {"WAVE", "Wave", GestureCategory::Dynamic, GestureAvailability::Planned},
    {"HOLD", "Hold gesture", GestureCategory::Temporal, GestureAvailability::Planned},
    {"DOUBLE_PINCH", "Double Pinch", GestureCategory::Temporal, GestureAvailability::Planned},
    {"DOUBLE_FIST", "Double Fist", GestureCategory::Temporal, GestureAvailability::Planned},
    {"SHORT_HOLD", "Short Hold", GestureCategory::Temporal, GestureAvailability::Planned},
    {"LONG_HOLD", "Long Hold", GestureCategory::Temporal, GestureAvailability::Planned},
}};

inline constexpr std::string_view gestureCategoryId(GestureCategory category)
{
    switch (category) {
    case GestureCategory::Static: return "static";
    case GestureCategory::Dynamic: return "dynamic";
    case GestureCategory::Temporal: return "temporal";
    }
    return "static";
}

inline constexpr std::string_view gestureAvailabilityId(GestureAvailability availability)
{
    switch (availability) {
    case GestureAvailability::Available: return "available";
    case GestureAvailability::Experimental: return "experimental";
    case GestureAvailability::Planned: return "planned";
    }
    return "planned";
}

inline constexpr bool isRecognizedGesture(GestureClass gesture)
{
    for (const auto &entry : kGestureRegistry) {
        if (entry.availability != GestureAvailability::Planned
            && entry.recognizedClass == gesture)
            return true;
    }
    return false;
}

inline constexpr bool isAvailableGestureId(std::string_view id)
{
    for (const auto &entry : kGestureRegistry) {
        if (entry.id == id)
            return entry.availability != GestureAvailability::Planned;
    }
    return false;
}

} // namespace vc
