#include "Data/SystemInfoProvider.hpp"
#include <array>
#include <cstdio>
#include <stdexcept>

// Whitelist команд — только строки из этого файла передаются в popen().
// Никакого пользовательского ввода сюда не попадает.
namespace {

struct ToolDef {
    const char* name;
    const char* versionCmd;   // команда для получения версии
    const char* statusCmd;    // команда для проверки статуса (nullptr = не нужна)
};

constexpr std::array<ToolDef, 8> TOOLS = {{
    { "OpenSSH",        "ssh -V 2>&1 | head -1",                          nullptr },
    { "Docker",         "docker --version 2>&1 | head -1",                nullptr },
    { "OpenSSL",        "openssl version 2>&1 | head -1",                 nullptr },
    { "rfkill",         "rfkill --version 2>&1 | head -1",                nullptr },
    { "NetworkManager", "NetworkManager --version 2>&1 | head -1",
                        "systemctl is-active NetworkManager 2>&1" },
    { "dhclient",       "dhclient --version 2>&1 | head -1",              nullptr },
    { "systemd",        "systemctl --version 2>&1 | head -1",             nullptr },
    { "iptables",       "iptables --version 2>&1 | head -1",              nullptr },
}};

} // namespace

SystemInfoProvider::SystemInfoProvider() = default;
SystemInfoProvider::~SystemInfoProvider() = default;

std::string SystemInfoProvider::runCommand(const char* cmd)
{
    std::array<char, 256> buf{};
    std::string result;

    FILE* pipe = ::popen(cmd, "r");
    if (!pipe) return {};

    if (::fgets(buf.data(), static_cast<int>(buf.size()), pipe))
        result = buf.data();

    ::pclose(pipe);

    // Убираем trailing newline
    if (!result.empty() && result.back() == '\n')
        result.pop_back();

    return result;
}

ToolInfo SystemInfoProvider::queryTool(const std::string& name,
                                        const char* versionCmd,
                                        const char* statusCmd)
{
    ToolInfo info;
    info.name = name;

    std::string ver = runCommand(versionCmd);

    if (ver.empty()) {
        info.version = "not installed";
        info.status  = ToolInfo::Status::NotInstalled;
        return info;
    }

    // Обрезаем версию до разумной длины
    if (ver.size() > 40) ver = ver.substr(0, 37) + "...";
    info.version = ver;

    // Если есть команда проверки статуса — используем её
    if (statusCmd) {
        std::string s = runCommand(statusCmd);
        info.status = (s == "active") ? ToolInfo::Status::Active
                                       : ToolInfo::Status::Inactive;
    } else {
        // Если утилита установлена но статус не проверяется — считаем активной
        info.status = ToolInfo::Status::Active;
    }

    return info;
}

void SystemInfoProvider::fetch()
{
    if (m_stopped) return;

    std::vector<ToolInfo> fresh;
    fresh.reserve(TOOLS.size());

    for (auto& def : TOOLS) {
        if (m_stopped) break;
        fresh.push_back(queryTool(def.name, def.versionCmd, def.statusCmd));
    }

    std::lock_guard lock(m_mutex);
    m_data = std::move(fresh);
}

void SystemInfoProvider::stop()
{
    m_stopped = true;
}

std::vector<ToolInfo> SystemInfoProvider::getData() const
{
    std::lock_guard lock(m_mutex);
    return m_data;
}
