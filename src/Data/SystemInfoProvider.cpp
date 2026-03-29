#include "App/Config.hpp"
#include "Data/SystemInfoProvider.hpp"
#include <array>
#include <cstdio>    // explicit — popen/fgets/pclose are C stdio, not guaranteed transitively
#include <sys/wait.h> // explicit — WEXITSTATUS is POSIX, never comes transitively

// command whitelist — nothing from user input ever touches popen()
namespace {

struct ToolDef {
    const char* name;
    const char* versionCmd;
    const char* statusCmd;  // nullptr means we dont check service status
};

constexpr std::array<ToolDef, 8> TOOLS = {{
    { "OpenSSH",        "ssh -V 2>&1 | head -1",                       nullptr },
    { "Docker",         "docker --version 2>&1 | head -1",             nullptr },
    { "OpenSSL",        "openssl version 2>&1 | head -1",              nullptr },
    { "rfkill",         "rfkill --version 2>&1 | head -1",             nullptr },
    { "NetworkManager", "NetworkManager --version 2>&1 | head -1",
                        "systemctl is-active NetworkManager 2>&1"              },
    { "dhclient",       "dhclient --version 2>&1 | head -1",           nullptr },
    { "systemd",        "systemctl --version 2>&1 | head -1",          nullptr },
    { "iptables",       "iptables --version 2>&1 | head -1",           nullptr },
}};

} // namespace

SystemInfoProvider::SystemInfoProvider() = default;
SystemInfoProvider::~SystemInfoProvider() = default;

SystemInfoProvider::CommandResult SystemInfoProvider::runCommand(const char* cmd)
{
    std::array<char, 256> buf{};
    CommandResult result;

    FILE* pipe = ::popen(cmd, "r");
    if (!pipe) return result;

    // grab first line only — we dont need the rest
    if (::fgets(buf.data(), static_cast<int>(buf.size()), pipe))
        result.output = buf.data();

    // pclose() returns exit status in waitpid format — WEXITSTATUS extracts the actual code
    // shell returns 127 if command not found, non-zero for other failures
    int raw = ::pclose(pipe);
    if (raw >= 0 && WIFEXITED(raw)) {
        result.exitCode = WEXITSTATUS(raw);
    }

    // common shell-level signals that command exists in whitelist but missing on host
    if (result.exitCode == 126 || result.exitCode == 127)
        result.missingBinary = true;

    // grab stderr hints that shell uses when binary can't be executed/found
    if (result.output.find("command not found") != std::string::npos ||
        result.output.find("not found") != std::string::npos ||
        result.output.find("No such file or directory") != std::string::npos) {
        result.missingBinary = true;
    }

    // strip trailing newline if present so comparisons stay exact
    if (!result.output.empty() && result.output.back() == '\n')
        result.output.pop_back();

    return result;
}

ToolInfo SystemInfoProvider::queryTool(const std::string& name,
                                        const char* versionCmd,
                                        const char* statusCmd)
{
    ToolInfo info;
    info.name = name;

    CommandResult verResult = runCommand(versionCmd);
    std::string& ver = verResult.output;

    // some shells hide the failing command exit behind a pipeline, but still print
    // a clear "not found" style message in the captured first line
    if (verResult.missingBinary) {
        info.version = "not installed";
        info.status  = ToolInfo::Status::NotInstalled;
        return info;
    }

    if (verResult.exitCode != 0) {
        info.version = "inactive";
        info.status  = verResult.missingBinary ? ToolInfo::Status::NotInstalled
                                               : ToolInfo::Status::Inactive;
        return info;
    }

    if (ver.empty()) {
        info.version = "not installed";
        info.status  = ToolInfo::Status::NotInstalled;
        return info;
    }

    // cap version string so it fits in the UI column
    if (ver.size() > Config::SYSTEM_INFO_VERSION_MAX_LEN) {
        ver = ver.substr(0, Config::SYSTEM_INFO_VERSION_MAX_LEN - 3) + "...";
    }
    info.version = ver;

    if (statusCmd) {
        CommandResult statusResult = runCommand(statusCmd);

        if (statusResult.missingBinary) {
            info.status = ToolInfo::Status::NotInstalled;
            return info;
        }

        if (statusResult.exitCode != 0) {
            info.status = statusResult.missingBinary ? ToolInfo::Status::NotInstalled
                                                     : ToolInfo::Status::Inactive;
            return info;
        }

        // systemctl prints "active" for running services, anything else is not active
        info.status = (statusResult.output == "active") ? ToolInfo::Status::Active
                                                        : ToolInfo::Status::Inactive;
    } else {
        // no status command means tool is just installed, treat as active
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
