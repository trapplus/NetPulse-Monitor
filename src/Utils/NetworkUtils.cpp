#include "Utils/NetworkUtils.hpp"
#include <string>
#include <arpa/inet.h>   // explicit - POSIX, never comes transitively

namespace NetUtils {

std::string hexToIP(const std::string& hex)
{
    // /proc/net/tcp stores IPs as little-endian hex, inet_ntoa handles the conversion
    unsigned long addr = std::stoul(hex, nullptr, 16);
    struct in_addr in{};
    in.s_addr = static_cast<in_addr_t>(addr);
    return std::string(inet_ntoa(in));
}

int hexToPort(const std::string& hex)
{
    return static_cast<int>(std::stoul(hex, nullptr, 16));
}

}
