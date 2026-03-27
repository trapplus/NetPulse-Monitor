#pragma once
#include <chrono>
#include <string>

struct RequestEntry {
    std::string method;
    std::string host;
    std::string path;
    std::string srcIP;
    std::string dstIP;
    int         dstPort { 0 };
    std::size_t payloadBytes { 0 };
    bool        isEncrypted { false };
    std::chrono::system_clock::time_point timestamp;
};
