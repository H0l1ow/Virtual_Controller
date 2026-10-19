#include "gestures/AsyncJsonlWriter.hpp"

#include <exception>
#include <utility>

namespace vc {

bool AsyncJsonlWriter::open(const std::filesystem::path &path)
{
    if (thread_.joinable())
        return false;
    {
        std::lock_guard lock(mutex_);
        error_.clear();
        failed_ = false;
        closing_ = false;
        pending_.clear();
    }
    stream_.open(path, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!stream_.is_open()) {
        std::lock_guard lock(mutex_);
        failed_ = true;
        error_ = "Cannot open recording file for writing";
        return false;
    }
    try {
        thread_ = std::thread([this] { writeLoop(); });
    } catch (const std::exception &exception) {
        stream_.close();
        std::lock_guard lock(mutex_);
        failed_ = true;
        error_ = std::string("Cannot start recording writer: ") + exception.what();
        return false;
    }
    return true;
}

bool AsyncJsonlWriter::enqueue(std::string line)
{
    {
        std::lock_guard lock(mutex_);
        if (!thread_.joinable() || closing_ || failed_)
            return false;
        if (pending_.size() >= kMaxQueuedLines) {
            failed_ = true;
            error_ = "Recording writer queue full: disk is too slow";
            // Previously accepted lines MUST still be drained on close().
            return false;
        }
        pending_.push_back(std::move(line));
    }
    changed_.notify_one();
    return true;
}

void AsyncJsonlWriter::writeLoop()
{
    for (;;) {
        std::deque<std::string> batch;
        {
            std::unique_lock lock(mutex_);
            changed_.wait(lock, [this] { return closing_ || !pending_.empty(); });
            if (pending_.empty()) {
                if (closing_) break;
                continue;
            }
            batch.swap(pending_);
        }
        for (const auto &line : batch) {
            stream_ << line;
            if (!stream_) {
                std::lock_guard lock(mutex_);
                failed_ = true;
                error_ = "Write failed: check storage space and permissions";
                pending_.clear();
                break;
            }
        }
        if (!stream_) break;
    }
    stream_.flush();
    if (!stream_) {
        std::lock_guard lock(mutex_);
        failed_ = true;
        if (error_.empty()) error_ = "Failed to flush recording to disk";
    }
    stream_.close();
}

void AsyncJsonlWriter::close()
{
    {
        std::lock_guard lock(mutex_);
        closing_ = true;
    }
    changed_.notify_all();
    if (thread_.joinable()) thread_.join();
    // A failed open() has no worker thread; its stream was closed in open().
}

bool AsyncJsonlWriter::isOpen() const
{
    std::lock_guard lock(mutex_);
    return thread_.joinable() && !closing_;
}

std::string AsyncJsonlWriter::error() const
{
    std::lock_guard lock(mutex_);
    return error_;
}

} // namespace vc
