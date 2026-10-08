#pragma once

#include "gestures/GestureTypes.hpp"

#include <array>
#include <string_view>

namespace vc {

enum class GestureCapability : std::uint8_t {
    StaticRules = 0,
    TemporalPlanned
};

struct GestureDefinition {
    GestureClass gesture{GestureClass::None};
    const char *displayName{};
    const char *category{};
    const char *description{};
    const char *glyph{};
    GestureCapability capability{GestureCapability::StaticRules};
};

inline constexpr std::array<GestureDefinition, kMappingGestureCount>
    kGestureDefinitions{{
        {GestureClass::Fist, "Fist", "Static", "Closed hand.", "fist", GestureCapability::StaticRules},
        {GestureClass::OpenHand, "Open Hand", "Static", "Open hand with all fingers extended.", "open", GestureCapability::StaticRules},
        {GestureClass::Point, "Point", "Static", "Index finger extended while the other fingers are curled.", "point", GestureCapability::StaticRules},
        {GestureClass::Pinch, "Pinch", "Static", "Thumb and index finger touch. Other fingers may be relaxed or curled.", "pinch", GestureCapability::StaticRules},
        {GestureClass::ThumbUp, "Thumb Up", "Static", "Thumb extended upward with the remaining fingers curled.", "up", GestureCapability::StaticRules},
        {GestureClass::ThumbDown, "Thumb Down", "Static", "Thumb extended downward with the remaining fingers curled.", "down", GestureCapability::StaticRules},
        {GestureClass::Victory, "Victory", "Static", "Index and middle fingers extended.", "two", GestureCapability::StaticRules},
        {GestureClass::Ok, "OK", "Static", "Thumb and index touch while middle, ring and little fingers stay extended.", "pinch", GestureCapability::StaticRules},
        {GestureClass::ILoveYou, "I Love You", "Static", "Thumb, index and little fingers extended.", "open", GestureCapability::StaticRules},
        {GestureClass::Rock, "Rock", "Static", "Index and little fingers extended with the thumb folded.", "two", GestureCapability::StaticRules},
        {GestureClass::CallMe, "Call Me", "Static", "Thumb and little finger extended.", "open", GestureCapability::StaticRules},
        {GestureClass::ThreeFingers, "Three Fingers", "Static", "Index, middle and ring fingers extended.", "open", GestureCapability::StaticRules},
        {GestureClass::FourFingers, "Four Fingers", "Static", "Four fingers extended with the thumb folded.", "open", GestureCapability::StaticRules},
        {GestureClass::SwipeLeft, "Swipe Left", "Dynamic", "Horizontal temporal gesture moving left.", "swipe", GestureCapability::TemporalPlanned},
        {GestureClass::SwipeRight, "Swipe Right", "Dynamic", "Horizontal temporal gesture moving right.", "swipe", GestureCapability::TemporalPlanned},
        {GestureClass::SwipeUp, "Swipe Up", "Dynamic", "Vertical temporal gesture moving up.", "swipe", GestureCapability::TemporalPlanned},
        {GestureClass::SwipeDown, "Swipe Down", "Dynamic", "Vertical temporal gesture moving down.", "swipe", GestureCapability::TemporalPlanned},
        {GestureClass::CircleCw, "Circle CW", "Dynamic", "Clockwise circular hand motion.", "rotate", GestureCapability::TemporalPlanned},
        {GestureClass::CircleCcw, "Circle CCW", "Dynamic", "Counter-clockwise circular hand motion.", "rotate", GestureCapability::TemporalPlanned},
        {GestureClass::Push, "Push", "Dynamic", "Hand moves toward the camera.", "open", GestureCapability::TemporalPlanned},
        {GestureClass::Pull, "Pull", "Dynamic", "Hand moves away from the camera.", "open", GestureCapability::TemporalPlanned}
    }};

inline constexpr const GestureDefinition *gestureDefinition(GestureClass gesture)
{
    for (const auto &definition : kGestureDefinitions) {
        if (definition.gesture == gesture) {
            return &definition;
        }
    }
    return nullptr;
}

inline constexpr const char *gestureDisplayName(GestureClass gesture)
{
    if (const auto *definition = gestureDefinition(gesture)) {
        return definition->displayName;
    }
    return "NONE";
}

inline constexpr bool gestureRuntimeAvailable(GestureClass gesture)
{
    const auto *definition = gestureDefinition(gesture);
    return definition
        && definition->capability == GestureCapability::StaticRules;
}

inline constexpr bool gestureTemporalPlanned(GestureClass gesture)
{
    const auto *definition = gestureDefinition(gesture);
    return definition
        && definition->capability == GestureCapability::TemporalPlanned;
}

} // namespace vc
