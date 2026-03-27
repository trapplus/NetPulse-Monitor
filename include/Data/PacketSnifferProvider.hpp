#pragma once
#include "Data/IDataProvider.hpp"
#include "Data/RequestEntry.hpp"
#include <atomic>
#include <mutex>
#include <pcap.h>
#include <string>
#include <vector>

class PacketSnifferProvider : public IDataProvider {
public:
    PacketSnifferProvider();
    ~PacketSnifferProvider() override;

    void fetch() override;
    void stop() override;

    std::vector<RequestEntry> getData() const;
    std::vector<std::string> getAvailableInterfaces() const;
    void setInterface(const std::string& name);

private:
    bool openHandle();
    void closeHandle();

    static void packetCallback(u_char* user,
                               const struct pcap_pkthdr* header,
                               const u_char* packet);
    void handlePacket(const struct pcap_pkthdr* header, const u_char* packet);

    static bool parseHTTP(const u_char* payload, std::size_t len,
                          const std::string& srcIP, const std::string& dstIP,
                          int dstPort, std::size_t payloadBytes,
                          RequestEntry& out);

    mutable std::mutex        m_mutex;
    std::vector<RequestEntry> m_entries;

    pcap_t*     m_handle   { nullptr };
    int         m_datalink { 0 };

    std::string m_interface;
    std::string m_requestedInterface;
    bool        m_reinitNeeded { false };

    std::vector<std::string> m_availableInterfaces;

    std::atomic<bool> m_stopped { false };
};
