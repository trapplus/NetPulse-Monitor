#pragma once
#include "Data/IDataProvider.hpp"
#include <string>
#include <vector>
#include <mutex>
#include <atomic>

struct ToolInfo {
    std::string name;
    std::string version;  // версия или статус строкой
    enum class Status { Active, Inactive, NotInstalled } status;
};

class SystemInfoProvider : public IDataProvider
{
public:
    SystemInfoProvider();
    ~SystemInfoProvider() override;

    void fetch() override;
    void stop() override;

    // Потокобезопасное чтение из Render-потока
    std::vector<ToolInfo> getData() const;

private:
    // Запускает команду и возвращает первую строку вывода.
    // Пустая строка = команда не найдена / ошибка.
    static std::string runCommand(const char* cmd);

    static ToolInfo queryTool(const std::string& name,
                              const char* versionCmd,
                              const char* statusCmd);

    mutable std::mutex        m_mutex;
    std::vector<ToolInfo>     m_data;
    std::atomic<bool>         m_stopped { false };
};
