#include "Utils/NetworkUtils.hpp"
#include <string>
#include <arpa/inet.h>
namespace NetUtils {

std::string hexToIP(const std::string& hex)
{
    unsigned long addr = std::stoul(hex, nullptr, 16);
    struct in_addr in{};
    in.s_addr = static_cast<in_addr_t>(addr);
    return inet_ntoa(in);
}

int hexToPort(const std::string& hex)
{
    return static_cast<int>(std::stoul(hex, nullptr, 16));
}

}
