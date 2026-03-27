#pragma once
#include "Data/PacketSnifferProvider.hpp"
#include <memory>

// forward declaration - we only store a pointer here, full type not needed in this header
// ApplicationController.cpp includes provider headers directly where methods are called
class SystemInfoProvider;
class NetworkDeviceProvider;
class ConnectionProvider;
class ExternalAPIProvider;
class PacketSnifferProvider;

// central place that owns all data providers
// ApplicationController creates and stores this, render methods read from it
// each provider has its own internal mutex - no global lock needed here
struct DataManager
{
    std::unique_ptr<SystemInfoProvider> systemInfo;
    std::unique_ptr<NetworkDeviceProvider> networkDevices;
    std::unique_ptr<ConnectionProvider> connections;
    std::unique_ptr<ExternalAPIProvider> externalAPI;
    std::unique_ptr<PacketSnifferProvider> packetSniffer;
};
