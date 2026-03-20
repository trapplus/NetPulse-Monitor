#pragma once
#include "Data/IDataProvider.hpp"
#include <atomic>
#include <mutex>
#include <string>
#include <vector>

// what we store per tool - name, version string, and one of three states
struct ToolInfo {
    std::string name;
    std::string version;
    enum class Status { Active, Inactive, NotInstalled } status;
};

// fetches versions and status of system tools via popen()
// only whitelisted commands are ever passed to popen - no user input
// getData() is safe to call from the render thread at any time
class SystemInfoProvider : public IDataProvider
{
public:
    SystemInfoProvider();
    ~SystemInfoProvider() override;

    void fetch() override;
    void stop()  override;

    // thread-safe read - returns a copy protected by the internal mutex
    std::vector<ToolInfo> getData() const;

private:
    // runs a shell command and returns the first line of output
    // returns empty string if the command wasnt found or failed
    static std::string runCommand(const char* cmd);

    static ToolInfo queryTool(const std::string& name,
                              const char* versionCmd,
                              const char* statusCmd);

    mutable std::mutex    m_mutex;
    std::vector<ToolInfo> m_data;
    std::atomic<bool>     m_stopped { false };
};
