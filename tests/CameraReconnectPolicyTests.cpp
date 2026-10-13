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
    std::cout << "Camera recovery policy: OK\n";
}
