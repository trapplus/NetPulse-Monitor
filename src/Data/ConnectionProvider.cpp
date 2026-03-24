#include "Data/ConnectionProvider.hpp"
#include "Utils/NetworkUtils.hpp"
#include <array>
#include <fstream>
#include <sstream>

ConnectionProvider::ConnectionProvider() = default;
ConnectionProvider::~ConnectionProvider() = default;

namespace {

bool parseIPv4(const std::string& ip, std::array<int, 4>& octets)
{
    std::istringstream in(ip);
    std::string part;
    int index = 0;

    while (std::getline(in, part, '.')) {
        if (index >= 4) return false;
        std::istringstream partStream(part);
        int value = -1;
        partStream >> value;
        if (partStream.fail() || !partStream.eof() || value < 0 || value > 255)
            return false;
        octets[static_cast<std::size_t>(index)] = value;
        ++index;
    }

    return index == 4;
}

} // namespace

void ConnectionProvider::parseProcNetFile(const char* path,
                                          ConnectionInfo::Protocol protocol,
                                          std::vector<ConnectionInfo>& out)
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
            conn.protocol = protocol;
            conn.status = parseStatus(stateHex);
        } catch (...) {
            // malformed rows do happen in proc snapshots, so we just skip them
            continue;
        }

        if (!isMeaningfulRemote(conn))
            continue;

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

bool ConnectionProvider::isMeaningfulRemote(const ConnectionInfo& conn)
{
    // keep only real machine-to-machine links; listeners or wildcard remotes are noise here
    if (conn.remotePort == 0 || conn.remoteIP == "0.0.0.0")
        return false;
    if (conn.remoteIP == conn.localIP)
        return false;

    std::array<int, 4> remoteOctets { 0, 0, 0, 0 };
    if (!parseIPv4(conn.remoteIP, remoteOctets))
        return false;

    // loopback, link-local, multicast and broadcast are not external peers
    if (remoteOctets[0] == 127)
        return false;
    if (remoteOctets[0] == 169 && remoteOctets[1] == 254)
        return false;
    if (remoteOctets[0] >= 224)
        return false;
    if (conn.remoteIP == "255.255.255.255")
        return false;

    // tcp graph should focus on active links only; LISTEN/TIME_WAIT are local socket states
    if (conn.protocol == ConnectionInfo::Protocol::TCP &&
        conn.status != ConnectionInfo::Status::ESTABLISHED) {
        return false;
    }

    return true;
}

void ConnectionProvider::fetch()
{
    if (m_stopped) return;

    std::vector<ConnectionInfo> fresh;
    parseProcNetFile("/proc/net/tcp", ConnectionInfo::Protocol::TCP, fresh);
    parseProcNetFile("/proc/net/udp", ConnectionInfo::Protocol::UDP, fresh);

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
