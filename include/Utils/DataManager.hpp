#pragma once
#include "Data/SystemInfoProvider.hpp"
#include <mutex>
#include <string>
#include <vector>
#include <memory>

// Централизованное хранилище провайдеров и данных.
// Render-поток читает через getData() методы провайдеров напрямую — под их mutex.
// ApplicationController владеет провайдерами через unique_ptr.
struct DataManager
{
    // Провайдеры — инициализируются в ApplicationController
    std::unique_ptr<SystemInfoProvider> systemInfo;

    // Заглушки для будущих провайдеров
    std::string externalIP;
    std::string isp;
    std::string location;
};
