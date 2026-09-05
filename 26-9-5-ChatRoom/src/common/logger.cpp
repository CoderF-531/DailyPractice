#include "chat/logger.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>

namespace chat {
namespace {

std::mutex& outputMutex() {
    static std::mutex mutex;
    return mutex;
}

const char* levelName(LogLevel level) {
    switch (level) {
        case LogLevel::Info:
            return "INFO";
        case LogLevel::Warn:
            return "WARN";
        case LogLevel::Error:
            return "ERROR";
    }
    return "UNKNOWN";
}

}  // namespace

void Logger::log(LogLevel level, std::string_view message) {
    const auto now = std::chrono::system_clock::now();
    const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm localTime{};
    localtime_r(&nowTime, &localTime);

    std::ostringstream line;
    line << '[' << std::put_time(&localTime, "%F %T") << "] [" << levelName(level) << "] [thread "
         << std::this_thread::get_id() << "] " << message;

    std::lock_guard<std::mutex> lock(outputMutex());
    std::cerr << line.str() << '\n';
}

void Logger::info(std::string_view message) {
    log(LogLevel::Info, message);
}

void Logger::warn(std::string_view message) {
    log(LogLevel::Warn, message);
}

void Logger::error(std::string_view message) {
    log(LogLevel::Error, message);
}

}  // namespace chat
