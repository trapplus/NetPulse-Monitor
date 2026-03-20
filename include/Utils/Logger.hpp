#pragma once
#include <string>

// basic logging to stdout - nothing fancy, just level-tagged output
// use Log::info for general flow, warn for recoverable issues, error for failures
namespace Log {
    void info (const std::string& msg);
    void warn (const std::string& msg);
    void error(const std::string& msg);
}
