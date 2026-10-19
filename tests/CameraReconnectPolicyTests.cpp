#include "runtime/CameraReconnectPolicy.hpp"

#include <cassert>
#include <iostream>

int main()
{
    using Policy = vc::CameraReconnectPolicy;
    Policy policy;
    assert(!policy.begin(""));
    assert(policy.begin("usb-camera-A", 4));
    assert(policy.pending());
    assert(policy.deviceId() == "usb-camera-A");
    // Another camera being available is not permission to restart.
    assert(policy.poll(false, true) == Policy::Decision::Wait);
    // Original camera returns while the worker is still stopping.
    assert(policy.poll(true, false) == Policy::Decision::Wait);
    assert(policy.poll(true, true) == Policy::Decision::Restart);
    assert(policy.poll(true, true) == Policy::Decision::Idle);

    assert(policy.begin("usb-camera-B", 2));
    assert(policy.poll(false, false) == Policy::Decision::Wait);
    assert(policy.poll(false, true) == Policy::Decision::TimedOut);
    assert(!policy.pending());

    assert(policy.begin("usb-camera-A", 1));
    assert(policy.poll(true, true) == Policy::Decision::Restart);
    assert(policy.begin("usb-camera-C"));
    policy.cancel();
    assert(policy.poll(true, true) == Policy::Decision::Idle);

    // When the user left system input ON, recovery remains pending beyond the
    // 30-second default window and waits specifically for the original device.
    assert(policy.begin("usb-camera-D", 1000));
    for (int i = 0; i < 90; ++i)
        assert(policy.poll(false, true) == Policy::Decision::Wait);
    assert(policy.pending());
    assert(policy.poll(true, true) == Policy::Decision::Restart);
    std::cout << "Camera recovery policy: OK\n";
}
