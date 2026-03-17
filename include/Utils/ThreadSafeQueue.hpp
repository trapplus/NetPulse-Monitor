#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

template<typename T>
class ThreadSafeQueue
{
public:
    void push(T value)
    {
        std::lock_guard lock(mutex_);
        queue_.push(std::move(value));
        cv_.notify_one();
    }

    std::optional<T> pop()
    {
        std::unique_lock lock(mutex_);
        cv_.wait(lock, [this]{ return !queue_.empty() || stopped_; });
        if (queue_.empty()) return std::nullopt;
        T val = std::move(queue_.front());
        queue_.pop();
        return val;
    }

    void stop()
    {
        std::lock_guard lock(mutex_);
        stopped_ = true;
        cv_.notify_all();
    }

private:
    std::queue<T>           queue_;
    std::mutex              mutex_;
    std::condition_variable cv_;
    bool                    stopped_ = false;
};
