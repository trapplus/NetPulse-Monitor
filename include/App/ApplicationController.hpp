#pragma once
#include <SFML/Graphics.hpp>
#include <thread>
#include <atomic>
#include "Utils/DataManager.hpp"

class ApplicationController {
public:
    ApplicationController();
    ~ApplicationController();
    void run();

private:
    void startDataThread();
    void stopDataThread();

    void processEvents();
    void update();
    void render();
    void renderPlaceholders();
    void renderSystemInfo();

    sf::RenderWindow m_window;
    sf::Font         m_font;
    bool             m_fontLoaded = false;

    DataManager      m_data;
    std::thread      m_dataThread;
    std::atomic<bool> m_running { true };

    sf::Clock        m_sysInfoClock;   // таймер обновления System Info
};
