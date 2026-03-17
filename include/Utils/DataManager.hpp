#pragma once
#include <mutex>
#include <string>
#include <vector>

// Централизованное хранилище с mutex.
// Расширяется по мере появления реальных структур данных.
struct DataManager
{
    std::mutex mtx;

    // Заглушки — будут заменены конкретными типами
    std::string externalIP;
    std::string isp;
    std::string location;
};
