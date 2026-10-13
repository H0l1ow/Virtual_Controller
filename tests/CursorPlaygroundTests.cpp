#include "control/CursorPlayground.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>

using namespace vc;

int main()
{
    CursorPlayground game;
    game.start(1'000'000);
    assert(game.active());
    assert(game.totalTargets() == 6);
    auto t = 1'000'000LL;
    game.update(0, 0, true, true, t + 33333); // first click outside
    assert(game.missedClicks() == 1);
    game.update(0, 0, true, false, t + 66666);

    // Moving toward the first target stays within the virtual plane.
    auto target = game.target();
    game.update((target.x-480)/2, (target.y-270)/2, true, false, t + 100000);
    game.update((target.x-480)/2, (target.y-270)/2, true, false, t + 133333);
    assert(game.completedTargets() == 0);
    assert(game.dwellPercent(t+680000) < 100);
    game.update(0, 0, true, false, t + 720000);
    assert(game.completedTargets() == 1);
    assert(game.averageAcquisitionMs() >= 700);
    assert(game.averageErrorPx() < 0.1);

    // A temporarily lost hand does not produce progress.
    game.update(100, 100, false, false, t + 790000);
    assert(!game.tracked());
    assert(game.completedTargets() == 1);

    for (int i=1; i<game.totalTargets(); ++i) {
        const auto p = game.target();
        auto cursor = game.cursor();
        t += 1'500'000;
        for (int step=0; step<4; ++step) {
            game.update((p.x - cursor.x)/4, (p.y - cursor.y)/4,
                        true, false, t+step*33333);
        }
        game.update(0, 0, true, false, t + 680000);
        assert(game.completedTargets() == i+1);
    }
    assert(game.complete());
    assert(!game.active());
    assert(game.missedClicks() == 1);
    game.stop();
}
