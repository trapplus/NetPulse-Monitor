#pragma once
#include "Data/IDataProvider.hpp"
#include <atomic>
#include <mutex>
#include <string>
#include <vector>

struct ConnectionInfo {
    enum class Status {
        ESTABLISHED,
        LISTEN,
        TIME_WAIT,
        OTHER
    };

    std::string localIP;
    int         localPort { 0 };
    std::string remoteIP;
    int         remotePort { 0 };
    Status      status { Status::OTHER };
};

class ConnectionProvider : public IDataProvider
{
public:
    ConnectionProvider();
    ~ConnectionProvider() override;

    void fetch() override;
    void stop() override;

    std::vector<ConnectionInfo> getData() const;

private:
    static void parseProcNetFile(const char* path, std::vector<ConnectionInfo>& out);
    static ConnectionInfo::Status parseStatus(const std::string& hexState);

    mutable std::mutex               m_mutex;
    std::vector<ConnectionInfo>      m_data;
    std::atomic<bool>                m_stopped { false };
};
