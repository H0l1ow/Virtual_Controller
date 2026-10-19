#include "gestures/AsyncJsonlWriter.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>

using namespace std::chrono_literals;

int main()
{
    namespace fs = std::filesystem;
    auto folder = fs::temp_directory_path() / "vc_async_jsonl_test";
    fs::create_directories(folder);
    auto file = folder / "recorded.jsonl";
    fs::remove(file);
    vc::AsyncJsonlWriter writer;
    assert(writer.open(file));
    assert(writer.isOpen());
    assert(!writer.open(file));  // Do not reopen an active recording.
    constexpr int kFrames = 120;
    for (int i = 0; i < kFrames; ++i) {
        assert(writer.enqueue("{\"sequence\":" + std::to_string(i) + "}\n"));
        if (i % 24 == 0) std::this_thread::sleep_for(1ms);
    }
    // Every successfully queued record must be durable after Stop.
    writer.close();
    assert(!writer.isOpen());
    std::ifstream input(file);
    assert(input);
    std::string line;
    for (int i = 0; i < kFrames; ++i) {
        assert(std::getline(input, line));
        assert(line == "{\"sequence\":" + std::to_string(i) + "}");
    }
    assert(!std::getline(input, line));
    fs::remove_all(folder);
}
