#pragma once
#include "Utils/DataManager.hpp"
#include <SFML/Graphics.hpp>
#include <atomic>
#include <thread>

// owns the SFML window, the data thread, and all providers via DataManager
// render methods must only be called from the main thread - SFML requirement
// data thread runs alongside and writes to providers under their own mutexes
class ApplicationController {
public:
    ApplicationController();
    ~ApplicationController();

    // enters the main loop - blocks until window is closed
    void run();

private:
    void startDataThread();
    void stopDataThread();   // sets m_running=false and joins

    void processEvents();
    void update();
    void render();

    // draws border frames for all 5 blocks - replaced block by block as providers land
    void renderPlaceholders();

    // reads from systemInfo provider and draws the tool table in block 1
    void renderSystemInfo();

    sf::RenderWindow  m_window;
    sf::Font          m_font;
    bool              m_fontLoaded = false;

    DataManager       m_data;
    std::thread       m_dataThread;
    std::atomic<bool> m_running { true };

    sf::Clock         m_sysInfoClock;  // tracks when to re-fetch system info
};
