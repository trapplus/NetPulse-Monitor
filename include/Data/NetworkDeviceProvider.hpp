#pragma once
#include "Data/IDataProvider.hpp"
#include <atomic>
#include <mutex>
#include <string>
#include <vector>

class NetworkDeviceProvider : public IDataProvider
{
public:
    NetworkDeviceProvider();
    ~NetworkDeviceProvider() override;

    void fetch() override;
    void stop() override;

private:
    struct DeviceInfo {
        std::string ip;
        std::string mac;
        std::string iface;
        std::string status;
    };

    mutable std::mutex      m_mutex;
    std::vector<DeviceInfo> m_data;
    std::atomic<bool>       m_stopped { false };
};
