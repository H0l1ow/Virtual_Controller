#include "input/OutputService.hpp"
#include "tracking/TrackingTypes.hpp"

#include <chrono>
#include <stdexcept>
#include <thread>

namespace {

template <typename Predicate>
void await(
    Predicate predicate,
    const char *message)
{
    const auto deadline =
        vc::nowUs() + 2'000'000;

    while (!predicate()) {
        if (vc::nowUs() > deadline) {
            throw std::runtime_error(message);
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(2));
    }
}

} // namespace

int main()
{
    vc::OutputService service(
        vc::OutputService::BackendKind::Mock);

    await(
        [&] {
            return service.status().ready;
        },
        "Mock backend did not initialize");

    if (service.status().armed) {
        throw std::runtime_error(
            "Output armed automatically");
    }

    if (!service.arm()) {
        throw std::runtime_error(
            "Could not arm mock output");
    }

    vc::ControllerState state;
    state.mouseX = 12.0F;
    service.submit(
        state,
        vc::nowUs(),
        1);

    await(
        [&] {
            return service.status().applied.mouseX != 0.0F;
        },
        "Mouse delta was not delivered");

    service.stop();

    await(
        [&] {
            return !service.status().armed
                && service.status().applied.neutral();
        },
        "STOP did not neutralize output");

    if (!service.arm()) {
        throw std::runtime_error(
            "Could not re-arm mock output");
    }

    service.submit(
        state,
        vc::nowUs(),
        2);

    // Producer freezes. OutputGuard timeout is 250 ms in the service.
    await(
        [&] {
            return !service.status().armed;
        },
        "Watchdog did not disarm stale output");

    if (!service.status().applied.neutral()) {
        throw std::runtime_error(
            "Watchdog left non-neutral output");
    }

    return 0;
}
