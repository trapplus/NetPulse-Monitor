#pragma once
#include "Data/SystemInfoProvider.hpp"
#include <memory>
#include <string>

// central place that owns all data providers
// ApplicationController creates and stores this, render methods read from it
// each provider has its own internal mutex - no global lock needed here
struct DataManager
{
    std::unique_ptr<SystemInfoProvider> systemInfo;

    // placeholders for providers not yet implemented
    std::string externalIP;
    std::string isp;
    std::string location;
};
