#include "gestures/HandFeatures.hpp"

#include <array>
#include <cassert>
#include <cmath>
#include <iostream>

namespace {
void near(float actual, float reference, float tolerance = 3.0e-5F)
{
    if (std::abs(actual - reference) > tolerance) {
        std::cerr << "feature mismatch: " << actual << " != " << reference << '\n';
        std::abort();
    }
}
}

int main()
{
    vc::HandTrackingState hand;
    hand.tracked = true;
    hand.side = vc::HandSide::Left;
    hand.reportedSide = vc::HandSide::Left;
    hand.trackId = 7;
    hand.handednessConfidence = 0.95F;
    for (int i = 0; i < 21; ++i) {
        hand.landmarks[static_cast<std::size_t>(i)] = {
            0.32F + static_cast<float>(i % 4) * 0.033F + static_cast<float>(i / 4) * 0.009F,
            0.27F + static_cast<float>(i % 5) * 0.049F + static_cast<float>(i / 5) * 0.006F,
            0.012F * static_cast<float>(i % 3)
        };
    }
    vc::HandFeatureExtractor features;
    const auto snapshot = features.update(hand, 640, 480, 1'000'000);
    // Frozen Python vcml.features geometry/feature reference values.
    near(snapshot[3], -0.267575F);
    near(snapshot[4], 0.297981F);
    near(snapshot[5], 0.097300F);
    near(snapshot[9], -0.802726F);
    near(snapshot[126], 0.446073F);
    near(snapshot[127], 0.754623F);
    near(snapshot[128], 0.754031F);
    near(snapshot[129], 0.734067F);
    near(snapshot[130], 0.288395F);
    near(snapshot[131], 0.551620F);
    near(snapshot[132], 0.95F);
    near(snapshot[133], 1.0F);
    assert(snapshot.size() == vc::kGestureFeatureCount);
    const auto g = vc::extractGestureGeometry(hand, 640, 480);
    assert(g.valid);
    const auto scores = vc::ruleGestureScores(g);
    near(scores[vc::gestureIndex(vc::GestureClass::Fist)], 1.0F);
    std::cout << "M6 native/Python 134-feature fixture contract: PASS\n";
    return 0;
}
