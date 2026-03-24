#include "Data/NetworkDeviceProvider.hpp"
#include <fstream>
#include <sstream>

NetworkDeviceProvider::NetworkDeviceProvider() = default;
NetworkDeviceProvider::~NetworkDeviceProvider() = default;

void NetworkDeviceProvider::fetch()
{
    if (m_stopped) return;

    std::ifstream arp("/proc/net/arp");
    if (!arp.is_open()) return;

    std::vector<NetworkDeviceInfo> fresh;
    std::string line;

    // first line is just column labels, so we skip it before parsing real rows
    if (!std::getline(arp, line)) return;

    while (std::getline(arp, line)) {
        if (m_stopped) break;

        std::istringstream row(line);
        std::string ip;
        std::string hwType;
        std::string flagsHex;
        std::string mac;
        std::string mask;
        std::string iface;

        // /proc/net/arp has fixed columns, so partial rows are ignored as invalid
        if (!(row >> ip >> hwType >> flagsHex >> mac >> mask >> iface))
            continue;

        unsigned int flags = 0;
        std::istringstream flagsStream(flagsHex);
        flagsStream >> std::hex >> flags;
        if (flagsStream.fail()) continue;

        NetworkDeviceInfo device;
        device.ip = std::move(ip);
        device.mac = std::move(mac);
        device.iface = std::move(iface);
        device.status = ((flags & 0x2U) != 0U) ? "Complete" : "Incomplete";
        fresh.push_back(std::move(device));
    }

    // swap the whole snapshot under lock so render thread sees a consistent table
    std::lock_guard lock(m_mutex);
    m_data = std::move(fresh);
}

void NetworkDeviceProvider::stop()
{
    m_stopped = true;
}

std::vector<NetworkDeviceInfo> NetworkDeviceProvider::getData() const
{
    std::lock_guard lock(m_mutex);
    return m_data;
}
