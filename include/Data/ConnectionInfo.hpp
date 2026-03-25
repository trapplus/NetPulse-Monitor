#pragma once
#include <string>

struct ConnectionInfo {
    enum class Protocol {
        TCP,
        UDP
    };

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
    Protocol    protocol { Protocol::TCP };
    Status      status { Status::OTHER };
};
