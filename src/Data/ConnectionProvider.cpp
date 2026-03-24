#include "Data/ConnectionProvider.hpp"
#include "Utils/NetworkUtils.hpp"
#include <fstream>
#include <sstream>

ConnectionProvider::ConnectionProvider() = default;
ConnectionProvider::~ConnectionProvider() = default;

void ConnectionProvider::parseProcNetFile(const char* path, std::vector<ConnectionInfo>& out)
{
    std::ifstream netFile(path);
    if (!netFile.is_open()) return;

    std::string line;

    // first row is header labels, so skip it before parsing real sockets
    if (!std::getline(netFile, line)) return;

    while (std::getline(netFile, line)) {
        std::istringstream row(line);

        std::string slot;
        std::string localAddr;
        std::string remoteAddr;
        std::string stateHex;

        // /proc/net/{tcp,udp} starts with: sl local_address rem_address st ...
        if (!(row >> slot >> localAddr >> remoteAddr >> stateHex))
            continue;

        const std::size_t localSep = localAddr.find(':');
        const std::size_t remoteSep = remoteAddr.find(':');
        if (localSep == std::string::npos || remoteSep == std::string::npos)
            continue;

        ConnectionInfo conn;
        try {
            conn.localIP = NetUtils::hexToIP(localAddr.substr(0, localSep));
            conn.localPort = NetUtils::hexToPort(localAddr.substr(localSep + 1));
            conn.remoteIP = NetUtils::hexToIP(remoteAddr.substr(0, remoteSep));
            conn.remotePort = NetUtils::hexToPort(remoteAddr.substr(remoteSep + 1));
            conn.status = parseStatus(stateHex);
        } catch (...) {
            // malformed rows do happen in proc snapshots, so we just skip them
            continue;
        }

        out.push_back(std::move(conn));
    }
}

ConnectionInfo::Status ConnectionProvider::parseStatus(const std::string& hexState)
{
    // kernel tcp states are hex, only the 3 we use for visuals get dedicated colors
    if (hexState == "01") return ConnectionInfo::Status::ESTABLISHED;
    if (hexState == "0A") return ConnectionInfo::Status::LISTEN;
    if (hexState == "06") return ConnectionInfo::Status::TIME_WAIT;
    return ConnectionInfo::Status::OTHER;
}

void ConnectionProvider::fetch()
{
    if (m_stopped) return;

    std::vector<ConnectionInfo> fresh;
    parseProcNetFile("/proc/net/tcp", fresh);
    parseProcNetFile("/proc/net/udp", fresh);

    std::lock_guard lock(m_mutex);
    m_data = std::move(fresh);
}

void ConnectionProvider::stop()
{
    m_stopped = true;
}

std::vector<ConnectionInfo> ConnectionProvider::getData() const
{
    std::lock_guard lock(m_mutex);
    return m_data;
}
