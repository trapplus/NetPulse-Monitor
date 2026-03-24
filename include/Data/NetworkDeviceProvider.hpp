#pragma once
#include "Data/IDataProvider.hpp"
#include <atomic>
#include <mutex>
#include <string>
#include <vector>

struct NetworkDeviceInfo {
    std::string ip;
    std::string mac;
    std::string iface;
    std::string status;
};

class NetworkDeviceProvider : public IDataProvider
{
public:
    NetworkDeviceProvider();
    ~NetworkDeviceProvider() override;

    void fetch() override;
    void stop() override;

    std::vector<NetworkDeviceInfo> getData() const;

private:
    mutable std::mutex             m_mutex;
    std::vector<NetworkDeviceInfo> m_data;
    std::atomic<bool>              m_stopped { false };
};
