#pragma once

#include <string_view>

namespace chat {

enum class LogLevel { Info, Warn, Error };

class Logger {
public:
    static void log(LogLevel level, std::string_view message);
    static void info(std::string_view message);
    static void warn(std::string_view message);
    static void error(std::string_view message);
};

}  // namespace chat
