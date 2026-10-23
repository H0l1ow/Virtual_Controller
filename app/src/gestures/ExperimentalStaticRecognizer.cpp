#include "gestures/ExperimentalStaticRecognizer.hpp"

#include <algorithm>
#include <cmath>

namespace vc {
namespace {

float distance(const Landmark &a, const Landmark &b)
{
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    const float dz = a.z - b.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

float rise(float value, float low, float high)
{
    return std::clamp((value - low) / (high - low), 0.0F, 1.0F);
}

float maximumConfidence(std::initializer_list<float> components)
{
    float result = 1.0F;
    for (const auto component : components)
        result = std::min(result, component);
    return result;
}

// Joint angles are scale/translation invariant and supplement the existing
// tip-to-base curl measure. In practice fingertips can be partly occluded;
// using both is less brittle than a single hard curl cutoff.
float jointAngle(const Landmark &a, const Landmark &b, const Landmark &c)
{
    const float ux = a.x - b.x, uy = a.y - b.y, uz = a.z - b.z;
    const float vx = c.x - b.x, vy = c.y - b.y, vz = c.z - b.z;
    const float norms = std::sqrt((ux * ux + uy * uy + uz * uz)
                                  * (vx * vx + vy * vy + vz * vz));
    if (norms < 1.0e-5F)
        return 0.0F;
    constexpr float kRadiansToDegrees = 57.2957795F;
    return std::acos(std::clamp((ux * vx + uy * vy + uz * vz) / norms,
                                -1.0F, 1.0F)) * kRadiansToDegrees;
}

float combinedFingerScore(float curlScore, float angleScore)
{
    // When the normalized curl strongly contradicts the joint angle, do not
    // turn an obviously wrong finger state into a confident template match.
    return curlScore < 0.20F ? 0.0F
                             : (0.75F * curlScore + 0.25F * angleScore);
}

} // namespace

GestureClass experimentalStaticClass(std::size_t index)
{
    return index < kExperimentalStaticCount
        ? static_cast<GestureClass>(gestureIndex(GestureClass::ThumbsUp) + index)
        : GestureClass::None;
}

ExperimentalStaticScores experimentalStaticScores(const GestureGeometry &g)
{
    ExperimentalStaticScores scores{};
    if (!g.valid)
        return scores;

    // The hand is already wrist-relative, scale-normalized and canonicalized
    // for handedness. Joint angles mitigate distorted curl ratios from camera
    // perspective; neither signal is a calibrated ML confidence.
    std::array<float, 4> straight{};
    std::array<float, 4> bent{};
    for (std::size_t i = 0; i < 4; ++i) {
        const int base = 5 + static_cast<int>(i) * 4;
        const float pipAngle = jointAngle(g.local[base], g.local[base + 1],
                                          g.local[base + 2]);
        const float dipAngle = jointAngle(g.local[base + 1], g.local[base + 2],
                                          g.local[base + 3]);
        const float flexion = std::min(pipAngle, dipAngle);
        const float extensionByCurl = 1.0F - rise(g.curl[i], 0.12F, 0.29F);
        const float bendByCurl = rise(g.curl[i], 0.20F, 0.43F);
        straight[i] = combinedFingerScore(extensionByCurl,
                                          rise(flexion, 138.0F, 166.0F));
        bent[i] = combinedFingerScore(bendByCurl,
                                      1.0F - rise(flexion, 115.0F, 155.0F));
    }

    Landmark palm{};
    for (int i : {0, 5, 9, 13, 17}) {
        palm.x += g.local[i].x / 5.0F;
        palm.y += g.local[i].y / 5.0F;
        palm.z += g.local[i].z / 5.0F;
    }
    const float thumbReach = distance(g.local[4], palm);
    const float thumbOut = rise(thumbReach, 0.72F, 1.03F);
    const float thumbIn = 1.0F - rise(thumbReach, 0.42F, 0.79F);

    // Thumbs up/down are relative to the image vertical, not the mirrored X
    // coordinate. Distinct orientation evidence prevents a fist with the
    // thumb resting diagonally across the index from becoming THUMBS_UP.
    const float thumbDx = std::abs(g.local[4].x - g.local[2].x);
    const float thumbDy = g.local[2].y - g.local[4].y;
    const float vertical = rise(std::abs(thumbDy) / std::max(thumbDx, 0.015F),
                                1.65F, 2.7F);
    const float thumbStraight = rise(jointAngle(g.local[2], g.local[3], g.local[4]),
                                     140.0F, 165.0F);
    const float fourBent = maximumConfidence({bent[0], bent[1], bent[2], bent[3]});
    // Using vertical extension rather than thumb-to-palm reach avoids
    // rejecting correct poses in common widescreen 16:9 camera frames.
    const float up = rise(thumbDy, 0.45F, 0.68F);
    const float down = rise(-thumbDy, 0.45F, 0.68F);
    scores[0] = maximumConfidence({fourBent, vertical, thumbStraight, up});
    scores[1] = maximumConfidence({fourBent, vertical, thumbStraight, down});

    const float separation = distance(g.local[8], g.local[12]);
    scores[2] = maximumConfidence({straight[0], straight[1], bent[2], bent[3],
                                   thumbIn, rise(separation, 0.32F, 0.40F)});
    scores[3] = maximumConfidence({straight[0], bent[1], bent[2], straight[3], thumbIn});
    scores[4] = maximumConfidence({straight[0], straight[1], straight[2], bent[3], thumbIn});
    scores[5] = maximumConfidence({straight[0], straight[1], straight[2], straight[3], thumbIn});
    scores[6] = maximumConfidence({bent[0], bent[1], bent[2], straight[3], thumbOut});

    // OK = a bent index/thumb loop, unlike PINCH's extended index finger.
    // Require three other extended fingers to avoid accidental mouse clicks.
    const float ring = 1.0F - rise(g.pinch[0], 0.13F, 0.25F);
    const float curvedIndex = rise(g.curl[0], 0.16F, 0.30F);
    scores[7] = maximumConfidence({ring, curvedIndex, straight[1], straight[2], straight[3]});
    return scores;
}

} // namespace vc
