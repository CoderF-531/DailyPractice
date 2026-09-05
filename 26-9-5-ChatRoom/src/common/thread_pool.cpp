#include "chat/thread_pool.h"

#include "chat/logger.h"

#include <algorithm>
#include <exception>
#include <string>

namespace chat {

ThreadPool& ThreadPool::instance() {
    static ThreadPool pool;
    return pool;
}

ThreadPool::ThreadPool() {
    const unsigned int available = std::thread::hardware_concurrency();
    const std::size_t workerCount = std::max<std::size_t>(2, available == 0 ? 2 : available);
    workers_.reserve(workerCount);
    for (std::size_t index = 0; index < workerCount; ++index) {
        workers_.emplace_back(&ThreadPool::workerLoop, this);
    }
    Logger::info("thread pool started with " + std::to_string(workerCount) + " workers");
}

ThreadPool::~ThreadPool() {
    stop();
}

bool ThreadPool::submit(std::function<void()> task) {
    if (!task) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopping_) {
            return false;
        }
        tasks_.push(std::move(task));
    }
    taskReady_.notify_one();
    return true;
}

void ThreadPool::stop() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopping_) {
            return;
        }
        stopping_ = true;
    }

    taskReady_.notify_all();
    for (std::thread& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    Logger::info("thread pool stopped");
}

void ThreadPool::workerLoop() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            // The predicate avoids spurious wakeups and lets workers drain queued tasks on shutdown.
            taskReady_.wait(lock, [this] { return stopping_ || !tasks_.empty(); });
            if (stopping_ && tasks_.empty()) {
                return;
            }
            task = std::move(tasks_.front());
            tasks_.pop();
        }

        try {
            task();
        } catch (const std::exception& exception) {
            Logger::error(std::string("thread pool task failed: ") + exception.what());
        } catch (...) {
            Logger::error("thread pool task failed with an unknown exception");
        }
    }
}

}  // namespace chat
