#pragma once

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>

namespace vc {

// Asynchronous append-only JSONL writer with bounded memory and flush-on-stop.
// Single producer (runtime GUI thread); one file I/O thread.  On saturation
// it fails loudly instead of silently skipping ML training samples.
class AsyncJsonlWriter final
{
public:
    static constexpr std::size_t kMaxQueuedLines = 256;

    AsyncJsonlWriter() = default;
    AsyncJsonlWriter(const AsyncJsonlWriter &) = delete;
    AsyncJsonlWriter &operator=(const AsyncJsonlWriter &) = delete;
    ~AsyncJsonlWriter() { close(); }

    bool open(const std::filesystem::path &path);
    bool enqueue(std::string line);
    void close(); // Drain pending lines and flush before returning.
    bool isOpen() const;
    std::string error() const;

private:
    void writeLoop();

    mutable std::mutex mutex_;
    std::condition_variable changed_;
    std::deque<std::string> pending_;
    std::ofstream stream_;
    std::thread thread_;
    bool closing_{};
    bool failed_{};
    std::string error_;
};

} // namespace vc
