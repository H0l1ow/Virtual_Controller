#include "input/OutputService.hpp"
#include "tracking/TrackingTypes.hpp"

#include <chrono>
#include <stdexcept>
#include <thread>

namespace {

template <typename Predicate>
void await(Predicate predicate, const char *message)
{
    const auto deadline = vc::nowUs() + 2'500'000;
    while (!predicate()) {
        if (vc::nowUs() > deadline)
            throw std::runtime_error(message);
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
}

void sendTracked(vc::OutputService &service,
                 std::uint64_t &sequence,
                 const vc::ControllerState &state,
                 bool tracked = true)
{
    service.submit(state, vc::nowUs(), ++sequence, tracked);
}

void waitForStableTracking(vc::OutputService &service,
                           std::uint64_t &sequence,
                           const vc::ControllerState &state)
{
    for (int i = 0; i < 8; ++i) {
        sendTracked(service, sequence, state);
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
    }
    await([&] { return service.status().armed; }, "Fresh tracking did not resume output");
}

} // namespace

int main()
{
    vc::OutputService service(vc::OutputService::BackendKind::Mock);
    await([&] { return service.status().ready; }, "Mock backend did not initialize");
    if (service.status().requested || service.status().armed)
        throw std::runtime_error("Output enabled without user consent");

    if (!service.arm())
        throw std::runtime_error("Could not request mock output");
    if (!service.status().requested || service.status().armed)
        throw std::runtime_error("Initial request must wait for fresh tracking");

    std::uint64_t seq = 0;
    vc::ControllerState state;
    vc::ControllerState pressed;
    pressed.mouseLeft = true;
    pressed.keys = 1;
    waitForStableTracking(service, seq, state);
    sendTracked(service, seq, pressed);
    await([&] { return service.status().applied.mouseLeft; },
          "Click state did not reach mock backend");

    // Temporary loss of every tracked hand: release physically, remember ON.
    sendTracked(service, seq, pressed, false);
    await([&] {
        const auto status = service.status();
        return !status.armed && status.requested && status.applied.neutral();
    }, "Missing hand should safely pause but retain user's ON request");
    for (int i = 0; i < 4; ++i)
        sendTracked(service, seq, pressed, false);
    if (service.status().armed)
        throw std::runtime_error("Absent hands unexpectedly re-enabled input");

    waitForStableTracking(service, seq, pressed);
    // The gesture was already held at the moment of resumption. It must not
    // cause an unintended click/key-down until the user releases it first.
    sendTracked(service, seq, pressed);
    if (service.status().applied.mouseLeft || service.status().applied.keys)
        throw std::runtime_error("A stale held gesture was replayed on resume");
    // Feed distinct capture timestamps with realistic frame spacing. Back-to-
    // back synthetic frames can share the same microsecond timestamp, which
    // OutputGuard correctly rejects as a duplicate.
    sendTracked(service, seq, state);
    std::this_thread::sleep_for(std::chrono::milliseconds(25));
    for (int i = 0; i < 3; ++i) {
        sendTracked(service, seq, pressed);
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
    await([&] { return service.status().applied.mouseLeft; },
          "Input did not resume automatically after releasing old gesture");

    // A producer stall must release held keys, *not* clear ON preference.
    await([&] {
        const auto status = service.status();
        return !status.armed && status.requested && status.applied.neutral();
    }, "Watchdog permanently disarmed output or left a held input");

    waitForStableTracking(service, seq, state);
    sendTracked(service, seq, pressed);
    await([&] { return service.status().applied.mouseLeft; },
          "Watchdog recovery required an extra manual arm click");

    // A temporary UI/mapping reset also keeps the opt-in preference.
    service.pause("Mapping updated");
    await([&] { return service.status().requested && !service.status().armed; },
          "Pause erased the user's requested state");
    waitForStableTracking(service, seq, state);

    // OFF/STOP is permanent until another explicit arm().
    service.stop();
    await([&] {
        const auto status = service.status();
        return !status.requested && !status.armed && status.applied.neutral();
    }, "STOP did not clear both desired and physical output");
    for (int i = 0; i < 8; ++i) {
        sendTracked(service, seq, state);
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
    if (service.status().requested || service.status().armed)
        throw std::runtime_error("Output restarted after explicit user STOP");

    return 0;
}
