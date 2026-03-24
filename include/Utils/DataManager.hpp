#pragma once
#include <memory>
#include <string>

// forward declaration - we only store a pointer here, full type not needed in this header
// ApplicationController.cpp includes SystemInfoProvider.hpp directly where methods are called
class SystemInfoProvider;
class NetworkDeviceProvider;

// central place that owns all data providers
// ApplicationController creates and stores this, render methods read from it
// each provider has its own internal mutex - no global lock needed here
struct DataManager
{
    std::unique_ptr<SystemInfoProvider> systemInfo;
    std::unique_ptr<NetworkDeviceProvider> networkDevices;

    // placeholders for providers not yet implemented
    std::string externalIP;
    std::string isp;
    std::string location;
};
