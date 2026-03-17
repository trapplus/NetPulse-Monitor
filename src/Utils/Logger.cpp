#include "Utils/Logger.hpp"
#include <iostream>
#include <chrono>

namespace Log {

static void print(const char* level, const std::string& msg)
{
    std::cout << "[" << level << "] " << msg << "\n";
}

void info (const std::string& msg) { print("INFO ", msg); }
void warn (const std::string& msg) { print("WARN ", msg); }
void error(const std::string& msg) { print("ERROR", msg); }

}
