#pragma once
#include <string>

namespace NetUtils {
    std::string hexToIP(const std::string& hex);   // парсинг /proc/net/tcp
    int         hexToPort(const std::string& hex);
}
