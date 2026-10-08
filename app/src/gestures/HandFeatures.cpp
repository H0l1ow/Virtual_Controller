#include "gestures/HandFeatures.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace vc {

namespace {

Landmark subtract(
    const Landmark &a,
    const Landmark &b)
{
    return {
        a.x - b.x,
        a.y - b.y,
        a.z - b.z
    };
}

Landmark multiply(
    const Landmark &value,
    float scalar)
{
    return {
        value.x * scalar,
        value.y * scalar,
        value.z * scalar
    };
}

float norm(const Landmark &value)
{
    return std::sqrt(
        value.x * value.x
        + value.y * value.y
        + value.z * value.z);
}

float distance(
    const Landmark &a,
    const Landmark &b)
{
    return norm(subtract(a, b));
}

float linearScore(
    float value,
    float low,
    float high)
{
    if (high <= low) {
        return 0.0F;
    }

    return std::clamp(
        (value - low) / (high - low),
        0.0F,
        1.0F);
}

bool finiteLandmark(const Landmark &point)
{
    return std::isfinite(point.x)
        && std::isfinite(point.y)
        && std::isfinite(point.z);
}

} // namespace

GestureGeometry extractGestureGeometry(
    const HandTrackingState &hand,
    int frameWidth,
    int frameHeight)
{
    GestureGeometry geometry;

    if (!hand.tracked
        || frameWidth <= 0
        || frameHeight <= 0
        || !std::isfinite(hand.handednessConfidence)
        || hand.handednessConfidence < 0.20F) {
        return geometry;
    }

    std::array<Landmark, kHandLandmarkCount> points{};
    const float aspect =
        static_cast<float>(frameHeight)
        / static_cast<float>(frameWidth);

    for (std::size_t index = 0;
         index < kHandLandmarkCount;
         ++index) {
        const auto source = hand.landmarks[index];

        if (!finiteLandmark(source)
            || source.x < -0.08F
            || source.x > 1.08F
            || source.y < -0.08F
            || source.y > 1.08F
            || std::abs(source.z) > 1.5F) {
            return {};
        }

        points[index] = {
            source.x,
            source.y * aspect,
            source.z
        };
    }

    geometry.scale =
        0.5F
        * (distance(points[0], points[9])
           + distance(points[5], points[17]));

    if (geometry.scale < 0.02F
        || geometry.scale > 0.65F) {
        return {};
    }

    Landmark palm{};
    for (const int index : {0, 5, 9, 13, 17}) {
        palm.x += points[index].x;
        palm.y += points[index].y;
        palm.z += points[index].z;
    }

    geometry.palm = multiply(palm, 0.2F);

    for (std::size_t index = 0;
         index < kHandLandmarkCount;
         ++index) {
        geometry.local[index] =
            multiply(
                subtract(points[index], points[0]),
                1.0F / geometry.scale);

        // Canonicalize left and right hands into one feature space so the
        // classifier does not need duplicated LEFT_* / RIGHT_* classes.
        if (hand.side == HandSide::Left) {
            geometry.local[index].x =
                -geometry.local[index].x;
        }
    }

    for (int finger = 0;
         finger < 4;
         ++finger) {
        const int base = 5 + finger * 4;

        const float chain =
            distance(points[base], points[base + 1])
            + distance(points[base + 1], points[base + 2])
            + distance(points[base + 2], points[base + 3]);

        if (chain < 0.003F) {
            return {};
        }

        geometry.curl[static_cast<std::size_t>(finger)] =
            std::clamp(
                1.0F
                - distance(points[base], points[base + 3])
                    / chain,
                0.0F,
                1.0F);
    }

    geometry.pinch[0] =
        distance(geometry.local[4], geometry.local[8]);
    geometry.pinch[1] =
        distance(geometry.local[4], geometry.local[12]);

    geometry.valid = true;
    return geometry;
}

GestureFeatures HandFeatureExtractor::update(
    const HandTrackingState &hand,
    int frameWidth,
    int frameHeight,
    std::int64_t sampleUs)
{
    GestureFeatures features{};

    const auto geometry =
        extractGestureGeometry(
            hand,
            frameWidth,
            frameHeight);

    if (!geometry.valid) {
        reset();
        return features;
    }

    const float deltaSeconds =
        static_cast<float>(sampleUs - lastUs_)
        * 1.0e-6F;

    const bool velocityValid =
        initialized_
        && deltaSeconds > 0.0F
        && deltaSeconds <= 0.15F;

    for (std::size_t index = 0;
         index < kHandLandmarkCount;
         ++index) {
        const auto point = geometry.local[index];
        const auto base = index * 3U;

        features[base] = point.x;
        features[base + 1U] = point.y;
        features[base + 2U] = point.z;

        if (velocityValid) {
            const auto delta =
                multiply(
                    subtract(point, previous_[index]),
                    1.0F / deltaSeconds);

            features[63U + base] =
                std::clamp(delta.x, -20.0F, 20.0F);
            features[64U + base] =
                std::clamp(delta.y, -20.0F, 20.0F);
            features[65U + base] =
                std::clamp(delta.z, -20.0F, 20.0F);
        }
    }

    std::copy(
        geometry.curl.begin(),
        geometry.curl.end(),
        features.begin() + 126);

    features[130] =
        std::min(geometry.pinch[0], 3.0F);
    features[131] =
        std::min(geometry.pinch[1], 3.0F);
    features[132] =
        std::clamp(
            hand.handednessConfidence,
            0.0F,
            1.0F);
    features[133] = 1.0F;

    previous_ = geometry.local;
    lastUs_ = sampleUs;
    initialized_ = true;

    return features;
}

void HandFeatureExtractor::reset()
{
    previous_ = {};
    lastUs_ = 0;
    initialized_ = false;
}

HandFeatureStream::HandFeatureStream(
    int windowSize,
    int sampleRate)
    : periodUs_(
          sampleRate > 0
              ? 1000000 / sampleRate
              : 0)
    , windowSize_(windowSize)
{
    if (windowSize < 2
        || windowSize > 120
        || sampleRate < 10
        || sampleRate > 120) {
        throw std::invalid_argument(
            "Invalid gesture temporal window");
    }
}

bool HandFeatureStream::push(
    const HandTrackingState &hand,
    int frameWidth,
    int frameHeight,
    std::int64_t timestampUs)
{
    if (!extractGestureGeometry(
            hand,
            frameWidth,
            frameHeight)
             .valid) {
        reset();
        return false;
    }

    if (previous_
        && timestampUs <= previous_->timestampUs) {
        return false;
    }

    if (previous_
        && timestampUs - previous_->timestampUs
            > kGestureMaxGapUs) {
        reset();
    }

    Observation current{
        hand,
        frameWidth,
        frameHeight,
        timestampUs
    };

    if (!previous_) {
        previous_ = current;
        nextSampleUs_ = timestampUs;
    }

    bool changed = false;

    while (nextSampleUs_ <= timestampUs) {
        const Observation &sample =
            nextSampleUs_ == timestampUs
            ? current
            : *previous_;

        window_.push_back(
            extractor_.update(
                sample.hand,
                sample.width,
                sample.height,
                nextSampleUs_));

        if (static_cast<int>(window_.size())
            > windowSize_) {
            window_.pop_front();
        }

        nextSampleUs_ += periodUs_;
        changed = true;
    }

    previous_ = current;
    return changed;
}

void HandFeatureStream::reset()
{
    extractor_.reset();
    previous_.reset();
    window_.clear();
    nextSampleUs_ = 0;
}

std::vector<float> HandFeatureStream::tensor() const
{
    std::vector<float> result;
    result.reserve(
        window_.size()
        * kGestureFeatureCount);

    for (const auto &sample : window_) {
        result.insert(
            result.end(),
            sample.begin(),
            sample.end());
    }

    return result;
}

GestureScores ruleGestureScores(
    const GestureGeometry &geometry)
{
    GestureScores scores{};
    scores[gestureIndex(GestureClass::None)] = 1.0F;

    if (!geometry.valid) {
        return scores;
    }

    const float indexCurl = geometry.curl[0];
    const float otherMin = std::min({
        geometry.curl[1],
        geometry.curl[2],
        geometry.curl[3]
    });
    const float maxCurl =
        *std::max_element(
            geometry.curl.begin(),
            geometry.curl.end());
    const float minCurl =
        *std::min_element(
            geometry.curl.begin(),
            geometry.curl.end());

    const float pinch =
        1.0F
        - linearScore(
            geometry.pinch[0],
            0.17F,
            0.43F);

    const float fist =
        linearScore(
            minCurl,
            0.30F,
            0.52F);

    const float open =
        1.0F
        - linearScore(
            maxCurl,
            0.05F,
            0.22F);

    const float indexExtended =
        1.0F
        - linearScore(
            indexCurl,
            0.08F,
            0.24F);

    const float othersCurled =
        linearScore(
            otherMin,
            0.24F,
            0.48F);

    const float point =
        indexExtended * othersCurled;

    scores[gestureIndex(GestureClass::Fist)] = fist;
    scores[gestureIndex(GestureClass::OpenHand)] = open;
    scores[gestureIndex(GestureClass::Point)] = point;
    scores[gestureIndex(GestureClass::Pinch)] = pinch;

    // Resolve the two most common geometric ambiguities before temporal
    // smoothing. A closed fist can put fingertips near the thumb; a pinch can
    // otherwise look like a partially open hand.
    if (fist > 0.55F) {
        scores[gestureIndex(GestureClass::OpenHand)] = 0.0F;
        scores[gestureIndex(GestureClass::Point)] = 0.0F;
        scores[gestureIndex(GestureClass::Pinch)] = 0.0F;
    }
    else if (pinch > 0.55F) {
        scores[gestureIndex(GestureClass::OpenHand)] = 0.0F;
        scores[gestureIndex(GestureClass::Point)] = 0.0F;
    }
    else if (point > 0.55F) {
        scores[gestureIndex(GestureClass::OpenHand)] = 0.0F;
    }

    const float bestGesture =
        *std::max_element(
            scores.begin() + 1,
            scores.end());

    scores[gestureIndex(GestureClass::None)] =
        std::clamp(
            1.0F - bestGesture,
            0.0F,
            1.0F);

    return scores;
}

} // namespace vc
