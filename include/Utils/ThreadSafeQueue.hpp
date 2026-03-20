#pragma once
#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

// a simple thread-safe queue used to pass data from provider threads to the render thread
// push() is called from data/api threads, pop() blocks until something arrives
// call stop() when shutting down so any blocked pop() wakes up and returns nullopt
template<typename T>
class ThreadSafeQueue
{
public:
    void push(T value)
    {
        std::lock_guard lock(mutex_);
        queue_.push(std::move(value));
        cv_.notify_one();  // wake up whoever is waiting in pop()
    }

    // blocks until an item is available or stop() is called
    // returns nullopt if the queue was stopped and is empty
    std::optional<T> pop()
    {
        std::unique_lock lock(mutex_);
        cv_.wait(lock, [this]{ return !queue_.empty() || stopped_; });
        if (queue_.empty()) return std::nullopt;
        T val = std::move(queue_.front());
        queue_.pop();
        return val;
    }

    // signals all waiting pop() calls to wake up and return nullopt
    // call this before joining any thread that blocks on pop()
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
