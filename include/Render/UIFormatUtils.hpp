#pragma once
#include "Data/NetworkDeviceProvider.hpp"
#include "Data/RequestEntry.hpp"
#include "Data/SystemInfoProvider.hpp"
#include <SFML/Graphics/Color.hpp>
#include <chrono>
#include <cstddef>
#include <string>

namespace Fmt {

std::string truncateText(const std::string& value, std::size_t maxLen);
std::string formatTimestamp(const std::chrono::system_clock::time_point& point);
sf::Color requestMethodColor(const RequestEntry& entry);
sf::Color toolStatusColor(ToolInfo::Status status);
const char* toolStatusLabel(ToolInfo::Status status);
bool isCompleteDevice(const std::string& status);

} // namespace Fmt
