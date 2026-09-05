#pragma once

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace chat {

class ThreadPool {
public:
    static ThreadPool& instance();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    // Returns false after stop() begins, so callers never enqueue work that cannot run.
    bool submit(std::function<void()> task);
    void stop();

private:
    ThreadPool();
    ~ThreadPool();
    void workerLoop();

    std::mutex mutex_;
    std::condition_variable taskReady_;
    std::queue<std::function<void()>> tasks_;
    std::vector<std::thread> workers_;
    bool stopping_ = false;
};

}  // namespace chat
