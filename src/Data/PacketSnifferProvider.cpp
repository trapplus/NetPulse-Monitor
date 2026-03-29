#include "Data/PacketSnifferProvider.hpp"
#include "App/Config.hpp"
#include "Utils/Logger.hpp"
#include <arpa/inet.h>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <netinet/in.h>
#include <pcap.h>
#include <string>
#include <utility>
#include <vector>

namespace {
constexpr std::size_t ETHERNET_HEADER_LEN = 14;
constexpr std::size_t LINUX_SLL_HEADER_LEN = 16;
constexpr std::size_t IPV4_MIN_HEADER_LEN = 20;
constexpr std::size_t TCP_MIN_HEADER_LEN = 20;
constexpr std::size_t IPV4_VERSION_SHIFT = 4;
constexpr std::size_t IPV4_VERSION_MASK = 0x0F;
constexpr std::size_t IPV4_PROTOCOL_OFFSET = 9;
constexpr std::size_t IPV4_SRC_OFFSET = 12;
constexpr std::size_t IPV4_DST_OFFSET = 16;
constexpr std::size_t TCP_DST_PORT_OFFSET = 2;
constexpr std::size_t TCP_DATA_OFFSET = 12;
constexpr int IPV4_VERSION = 4;
constexpr int TCP_PROTOCOL = 6;

bool startsWith(const u_char* payload, std::size_t len, const char* pattern, std::size_t patternLen)
{
    return len >= patternLen && std::memcmp(payload, pattern, patternLen) == 0;
}

std::string truncateValue(const std::string& value, std::size_t maxLen)
{
    if (value.size() <= maxLen) {
        return value;
    }

    return value.substr(0, maxLen);
}

} // namespace

PacketSnifferProvider::PacketSnifferProvider()
{
    char errbuf[PCAP_ERRBUF_SIZE] = {};
    pcap_if_t* allDevices = nullptr;

    if (pcap_findalldevs(&allDevices, errbuf) == 0 && allDevices != nullptr) {
        // pcap_findalldevs returns everything including bluetooth, nflog, and dbus pseudo-interfaces
        // that don't support BPF — probe each one and keep only those we can actually parse
        for (pcap_if_t* dev = allDevices; dev != nullptr; dev = dev->next) {
            if (dev->name == nullptr) {
                continue;
            }

            char probeErr[PCAP_ERRBUF_SIZE] = {};
            pcap_t* probe = pcap_open_live(
                dev->name,
                Config::PCAP_PROBE_SNAP_LEN,
                Config::PCAP_PROMISCUOUS,
                Config::PCAP_PROBE_TIMEOUT_MS,
                probeErr
            );
            if (probe == nullptr) {
                continue;
            }

            const int dlt = pcap_datalink(probe);
            pcap_close(probe);
            if (dlt != DLT_EN10MB && dlt != DLT_LINUX_SLL) {
                continue;
            }

            m_availableInterfaces.emplace_back(dev->name);
        }

        pcap_freealldevs(allDevices);
    } else {
        Log::warn(std::string("PacketSnifferProvider: pcap_findalldevs failed: ") + errbuf);
        if (allDevices != nullptr) {
            pcap_freealldevs(allDevices);
        }
    }

    if (!m_availableInterfaces.empty()) {
        m_interface = m_availableInterfaces[0];
    } else {
        m_interface = "any";
    }

    // Open once here so the worker thread can start collecting right away and we fail fast on bad permissions.
    openHandle();
}

PacketSnifferProvider::~PacketSnifferProvider()
{
    closeHandle();
}

bool PacketSnifferProvider::openHandle()
{
    closeHandle();

    char errbuf[PCAP_ERRBUF_SIZE] = {};
    // Promiscuous off keeps traffic focused on local host paths, which is all the request log needs.
    // 100ms timeout lets libpcap wake up frequently so the worker loop stays responsive when traffic is quiet.
    m_handle = pcap_open_live(
        m_interface.c_str(),
        Config::PCAP_SNAP_LEN,
        Config::PCAP_PROMISCUOUS,
        Config::PCAP_READ_TIMEOUT_MS,
        errbuf
    );

    if (m_handle == nullptr) {
        Log::warn(std::string("PacketSnifferProvider: pcap_open_live failed for '") + m_interface + "': " + errbuf);
        return false;
    }

    // Non-blocking is required so fetch() never stalls the data thread when no packets are available.
    if (pcap_setnonblock(m_handle, 1, errbuf) != 0) {
        Log::warn(std::string("PacketSnifferProvider: pcap_setnonblock failed: ") + errbuf);
        closeHandle();
        return false;
    }

    m_datalink = pcap_datalink(m_handle);

    bpf_program filterProgram {};
    if (pcap_compile(m_handle, &filterProgram, Config::PCAP_BPF_FILTER, 1, PCAP_NETMASK_UNKNOWN) != 0) {
        Log::warn(std::string("PacketSnifferProvider: pcap_compile failed: ") + pcap_geterr(m_handle));
        closeHandle();
        return false;
    }

    // freecode is always needed after compile, even if setfilter fails, otherwise we leak BPF resources.
    const bool setFilterOk = pcap_setfilter(m_handle, &filterProgram) == 0;
    pcap_freecode(&filterProgram);

    if (!setFilterOk) {
        Log::warn(std::string("PacketSnifferProvider: pcap_setfilter failed: ") + pcap_geterr(m_handle));
        closeHandle();
        return false;
    }

    return true;
}

void PacketSnifferProvider::closeHandle()
{
    if (m_handle != nullptr) {
        pcap_close(m_handle);
        m_handle = nullptr;
    }
}

void PacketSnifferProvider::fetch()
{
    if (m_stopped.load()) {
        return;
    }

    bool shouldReopen = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_reinitNeeded) {
            closeHandle();
            m_interface = m_requestedInterface;
            m_reinitNeeded = false;
            // Defer the reopen until after we release m_mutex to avoid holding it across libpcap setup work.
            shouldReopen = true;
        }
    }

    if (shouldReopen) {
        openHandle();
    }

    if (m_handle == nullptr) {
        return;
    }

    // dispatch with a fixed packet budget keeps each tick bounded; loop() would block and starve other providers.
    pcap_dispatch(
        m_handle,
        Config::PCAP_MAX_PACKETS_PER_DISPATCH,
        packetCallback,
        reinterpret_cast<u_char*>(this)
    );
}

void PacketSnifferProvider::stop()
{
    m_stopped = true;
}

std::vector<RequestEntry> PacketSnifferProvider::getData() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_entries;
}

std::vector<std::string> PacketSnifferProvider::getAvailableInterfaces() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_availableInterfaces;
}

void PacketSnifferProvider::setInterface(const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_requestedInterface = name;
    m_reinitNeeded = true;
}

void PacketSnifferProvider::packetCallback(
    u_char* user,
    const struct pcap_pkthdr* header,
    const u_char* packet
)
{
    if (user == nullptr || header == nullptr || packet == nullptr) {
        return;
    }

    auto* self = reinterpret_cast<PacketSnifferProvider*>(user);
    self->handlePacket(header, packet);
}

void PacketSnifferProvider::handlePacket(const struct pcap_pkthdr* header, const u_char* packet)
{
    std::size_t linkOffset = 0;
    // Link-layer headers differ by capture source, so we normalize offsets before touching IP bytes.
    if (m_datalink == DLT_EN10MB) {
        linkOffset = ETHERNET_HEADER_LEN;
    } else if (m_datalink == DLT_LINUX_SLL) {
        linkOffset = LINUX_SLL_HEADER_LEN;
    } else {
        return;
    }

    if (header->caplen < linkOffset + IPV4_MIN_HEADER_LEN) {
        return;
    }

    const u_char* ipBase = packet + linkOffset;
    if ((ipBase[0] >> IPV4_VERSION_SHIFT) != IPV4_VERSION) {
        return;
    }

    const std::size_t ipHeaderLen = static_cast<std::size_t>(ipBase[0] & IPV4_VERSION_MASK) * 4;
    if (ipBase[IPV4_PROTOCOL_OFFSET] != TCP_PROTOCOL) {
        return;
    }

    char srcAddress[INET_ADDRSTRLEN] = {};
    char dstAddress[INET_ADDRSTRLEN] = {};
    inet_ntop(AF_INET, ipBase + IPV4_SRC_OFFSET, srcAddress, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, ipBase + IPV4_DST_OFFSET, dstAddress, INET_ADDRSTRLEN);

    if (header->caplen < linkOffset + ipHeaderLen + TCP_MIN_HEADER_LEN) {
        return;
    }

    const u_char* tcpBase = packet + linkOffset + ipHeaderLen;
    uint16_t rawDstPort = 0;
    // memcpy avoids undefined behavior from unaligned uint16_t reads on architectures with strict alignment.
    std::memcpy(&rawDstPort, tcpBase + TCP_DST_PORT_OFFSET, sizeof(rawDstPort));
    const int dstPort = static_cast<int>(ntohs(rawDstPort));
    const std::size_t tcpHeaderLen = static_cast<std::size_t>((tcpBase[TCP_DATA_OFFSET] >> IPV4_VERSION_SHIFT) & IPV4_VERSION_MASK) * 4;

    const std::size_t payloadOffset = linkOffset + ipHeaderLen + tcpHeaderLen;
    if (payloadOffset >= header->caplen) {
        // No payload means control packet or truncated capture, so there is nothing to parse as HTTP.
        return;
    }

    const std::size_t payloadLen = header->caplen - payloadOffset;

    RequestEntry entry;
    if (!parseHTTP(
            packet + payloadOffset,
            payloadLen,
            srcAddress,
            dstAddress,
            dstPort,
            payloadLen,
            entry
        )) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    m_entries.push_back(std::move(entry));
    if (m_entries.size() > Config::REQUEST_LOG_LIMIT) {
        m_entries.erase(m_entries.begin());
    }
}

bool PacketSnifferProvider::parseHTTP(
    const u_char* payload,
    std::size_t len,
    const std::string& srcIP,
    const std::string& dstIP,
    int dstPort,
    std::size_t payloadBytes,
    RequestEntry& out
)
{
    out.srcIP = srcIP;
    out.dstIP = dstIP;
    out.dstPort = dstPort;
    out.payloadBytes = payloadBytes;
    out.timestamp = std::chrono::system_clock::now();

    if (dstPort == Config::HTTPS_PORT) {
        // HTTPS payload is encrypted, but we still keep a lightweight row so users see encrypted traffic volume.
        out.isEncrypted = true;
        out.method.clear();
        out.path.clear();
        out.host = dstIP;
        out.dstPort = Config::HTTPS_PORT;
        return true;
    }

    if (len < Config::HTTP_MIN_METHOD_BYTES) {
        return false;
    }

    constexpr struct {
        const char* name;
        std::size_t len;
    } methods[] = {
        { "GET ", 4 },
        { "POST ", 5 },
        { "PUT ", 4 },
        { "DELETE ", 7 },
        { "PATCH ", 6 },
        { "HEAD ", 5 },
        { "OPTIONS ", 8 }
    };

    bool recognized = false;
    for (const auto& method : methods) {
        // Payload is raw bytes, so memcmp-based prefix checks are safer than c-string APIs.
        if (startsWith(payload, len, method.name, method.len)) {
            recognized = true;
            break;
        }
    }

    if (!recognized) {
        return false;
    }

    out.isEncrypted = false;

    std::size_t firstSpace = 0;
    while (firstSpace < len && payload[firstSpace] != ' ') {
        ++firstSpace;
    }
    if (firstSpace == len) {
        return false;
    }
    out.method.assign(reinterpret_cast<const char*>(payload), firstSpace);

    std::size_t secondSpace = firstSpace + 1;
    while (secondSpace < len && payload[secondSpace] != ' ') {
        ++secondSpace;
    }
    if (secondSpace > firstSpace + 1) {
        const auto pathLen = secondSpace - (firstSpace + 1);
        out.path.assign(reinterpret_cast<const char*>(payload + firstSpace + 1), pathLen);
        out.path = truncateValue(out.path, Config::REQUEST_LOG_MAX_PATH_LEN);
    } else {
        out.path.clear();
    }

    // If Host is absent (or malformed), dst IP is still a useful fallback for the request log row.
    out.host = dstIP;
    constexpr char hostHeader[] = "Host:";
    constexpr std::size_t hostHeaderLen = sizeof(hostHeader) - 1;

    for (std::size_t i = 0; i + hostHeaderLen < len; ++i) {
        if (std::memcmp(payload + i, hostHeader, hostHeaderLen) != 0) {
            continue;
        }

        std::size_t valueStart = i + hostHeaderLen;
        while (valueStart < len && (payload[valueStart] == ' ' || payload[valueStart] == '\t')) {
            ++valueStart;
        }

        std::size_t valueEnd = valueStart;
        while (valueEnd < len && payload[valueEnd] != '\r') {
            ++valueEnd;
        }

        if (valueEnd > valueStart) {
            out.host.assign(reinterpret_cast<const char*>(payload + valueStart), valueEnd - valueStart);
            out.host = truncateValue(out.host, Config::REQUEST_LOG_MAX_HOST_LEN);
        }
        break;
    }

    return true;
}
