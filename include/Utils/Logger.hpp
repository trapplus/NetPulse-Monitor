#pragma once
#include <string>

namespace Log {
    void info (const std::string& msg);
    void warn (const std::string& msg);
    void error(const std::string& msg);
}
