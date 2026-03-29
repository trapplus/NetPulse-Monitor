#include "Render/UIFormatUtils.hpp"
#include "App/Config.hpp"
#include <ctime>
#include <iomanip>
#include <sstream>

namespace Fmt {

std::string truncateText(const std::string& value, std::size_t maxLen)
{
    // Request-log columns are fixed width, so clipping here avoids layout jitter frame to frame.
    if (value.size() <= maxLen) {
        return value;
    }

    if (maxLen <= 3) {
        return value.substr(0, maxLen);
    }

    return value.substr(0, maxLen - 3) + "...";
}

std::string formatTimestamp(const std::chrono::system_clock::time_point& point)
{
    const std::time_t timeValue = std::chrono::system_clock::to_time_t(point);
    std::tm localTime {};
#if defined(_WIN32)
    localtime_s(&localTime, &timeValue);
#else
    // localtime_r keeps this safe while worker threads keep updating providers.
    localtime_r(&timeValue, &localTime);
#endif

    std::ostringstream stream;
    stream << std::put_time(&localTime, "%H:%M:%S");
    return stream.str();
}

sf::Color requestMethodColor(const RequestEntry& entry)
{
    // Keep request-method palette centralized so badges stay consistent across render paths.
    if (entry.isEncrypted) {
        return Config::TEXT_DIM_COLOR;
    }

    if (entry.method == "GET") {
        return Config::STATUS_ACTIVE_COLOR;
    }
    if (entry.method == "POST") {
        return Config::REQUEST_LOG_POST_COLOR;
    }
    if (entry.method == "PUT") {
        return Config::STATUS_INACTIVE_COLOR;
    }
    if (entry.method == "DELETE") {
        return Config::REQUEST_LOG_DELETE_COLOR;
    }
    if (entry.method == "PATCH") {
        return Config::REQUEST_LOG_PATCH_COLOR;
    }
    if (entry.method == "HEAD" || entry.method == "OPTIONS") {
        return Config::REQUEST_LOG_OPTIONS_COLOR;
    }

    return Config::TEXT_DIM_COLOR;
}

sf::Color toolStatusColor(ToolInfo::Status status)
{
    // Status shades are semantic, so mapping is shared instead of hand-written in each block.
    switch (status) {
        case ToolInfo::Status::Active: return Config::STATUS_ACTIVE_COLOR;
        case ToolInfo::Status::Inactive: return Config::STATUS_INACTIVE_COLOR;
        case ToolInfo::Status::NotInstalled: return Config::STATUS_NOT_INSTALLED_COLOR;
    }
    return Config::TEXT_PRIMARY_COLOR;
}

const char* toolStatusLabel(ToolInfo::Status status)
{
    switch (status) {
        case ToolInfo::Status::Active: return "[active]";
        case ToolInfo::Status::Inactive: return "[inactive]";
        case ToolInfo::Status::NotInstalled: return "[not installed]";
    }
    return "";
}

bool isCompleteDevice(const std::string& status)
{
    return status == "Complete";
}

} // namespace Fmt
