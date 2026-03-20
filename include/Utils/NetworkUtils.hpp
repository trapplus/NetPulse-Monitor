#pragma once
#include <string>

// helpers for parsing /proc/net/tcp and /proc/net/udp
// the kernel stores addresses and ports as little-endian hex strings
// e.g. "0101A8C0" -> "192.168.1.1",  "0050" -> 80
namespace NetUtils {
    std::string hexToIP(const std::string& hex);
    int         hexToPort(const std::string& hex);
}
