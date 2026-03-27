#pragma once
#include "Utils/DataManager.hpp"
#include <SFML/Graphics.hpp>
#include <atomic>
#include <thread>
#include <vector>

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
    void startWorkers();
    void stopWorkers();

    void processEvents();
    void update();
    void render();

    // draws border frames for all 5 blocks - replaced block by block as providers land
    void renderPlaceholders();

    // reads from systemInfo provider and draws the tool table in block 1
    void renderSystemInfo();
    void renderRequestLog();
    // reads from networkDevices provider and draws ARP rows in block 4
    void renderNetworkDevices();
    // reads from externalAPI provider and draws external IP/ISP/location in block 3
    void renderExternalAPI();
    // draws the connection graph and protocol counters in block 5
    void renderConnections();

    sf::RenderWindow  m_window;
    sf::Font          m_font;
    bool              m_fontLoaded = false;

    DataManager       m_data;
    std::thread       m_dataThread;
    std::thread       m_apiThread;
    std::atomic<bool> m_running { true };

    std::vector<std::string>   m_interfaces;
    std::size_t                m_selectedInterface { 0 };
    std::vector<sf::FloatRect> m_ifaceButtonBounds;
    std::size_t                m_selectedVisualizerMode { 0 };
    std::vector<sf::FloatRect> m_visualizerModeButtonBounds;
};
