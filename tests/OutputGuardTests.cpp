#include "input/InputBackend.hpp"
#include "input/OutputGuard.hpp"

#include <cassert>

int main()
{
    vc::OutputGuard guard(150000);
    vc::ControllerState state;
    state.mouseX = 5.0F;
    state.mouseY = -3.0F;
    state.mouseLeft = true;

    // Disarmed output ignores producer submissions.
    guard.submit(state, 1000, 1);
    assert(guard.poll(1001).neutral());

    guard.arm();
    guard.submit(state, 2000, 2);

    const auto first = guard.poll(2001);
    assert(first.mouseX == 5.0F);
    assert(first.mouseY == -3.0F);
    assert(first.mouseLeft);

    // Relative movement is one-shot, held state is not.
    const auto second = guard.poll(2002);
    assert(second.mouseX == 0.0F);
    assert(second.mouseY == 0.0F);
    assert(second.mouseLeft);

    // Older tracking results cannot replace newer output.
    vc::ControllerState older;
    older.mouseX = 99.0F;
    guard.submit(older, 1999, 1);
    assert(guard.poll(2003).mouseX == 0.0F);

    // Stale producer data automatically disarms and neutralizes.
    assert(guard.poll(152001).neutral());
    assert(!guard.armed());

    vc::MockInputBackend mock;
    mock.apply(state);
    assert(mock.last().mouseLeft);
    assert(mock.applyCount() == 1);
    mock.releaseAll();
    assert(mock.last().neutral());
    assert(mock.releaseCount() == 1);

    return 0;
}
