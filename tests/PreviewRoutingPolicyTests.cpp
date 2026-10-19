#include "runtime/PreviewRoutingPolicy.hpp"

#include <cassert>
#include <iostream>

int main()
{
    using vc::PreviewTarget;
    using vc::selectPreviewTarget;

    assert(selectPreviewTarget(false, false, false, false) == PreviewTarget::None);
    assert(selectPreviewTarget(true, false, false, false) == PreviewTarget::Controller);
    assert(selectPreviewTarget(false, true, false, false) == PreviewTarget::Recording);
    assert(selectPreviewTarget(false, false, true, false) == PreviewTarget::None);
    assert(selectPreviewTarget(true, true, true, false) == PreviewTarget::Recording);
    assert(selectPreviewTarget(true, true, true, true) == PreviewTarget::PiP);
    assert(selectPreviewTarget(true, true, false, true) == PreviewTarget::None);
    // Minimize/Show Desktop suppresses rendering in *every* ordinary tab.
    // It does not require disabling the capture sink / tracking.
    assert(selectPreviewTarget(true, true, false, false, true) == PreviewTarget::None);
    assert(selectPreviewTarget(true, true, true, false, true) == PreviewTarget::None);
    assert(selectPreviewTarget(true, true, true, true, true) == PreviewTarget::PiP);
    assert(selectPreviewTarget(false, false, false, false, true) == PreviewTarget::None);

    // All 16 combinations select no more than a single renderer.
    for (int bits = 0; bits < 16; ++bits) {
        const auto target = selectPreviewTarget(bits & 1, bits & 2, bits & 4, bits & 8);
        assert(target == PreviewTarget::None || target == PreviewTarget::Controller
               || target == PreviewTarget::Recording || target == PreviewTarget::PiP);
        if (bits & 8) {
            assert(target == ((bits & 4) ? PreviewTarget::PiP : PreviewTarget::None));
        }
    }
    // Check every attachment and minimized state combination.
    for (int bits = 0; bits < 32; ++bits) {
        const bool controller = (bits & 1) != 0;
        const bool recording = (bits & 2) != 0;
        const bool pip = (bits & 4) != 0;
        const bool pipActive = (bits & 8) != 0;
        const bool minimized = (bits & 16) != 0;
        const auto target = selectPreviewTarget(controller, recording,
                                                 pip, pipActive, minimized);
        if (minimized) {
            assert(target == ((pipActive && pip)
                       ? PreviewTarget::PiP : PreviewTarget::None));
        }
    }
    std::cout << "Exclusive preview target routing passed\n";
}
