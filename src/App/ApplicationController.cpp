#include "App/ApplicationController.hpp"
#include "App/Config.hpp"
#include "Data/SystemInfoProvider.hpp"  // explicit — DataManager.hpp only forward-declares it
#include <array>
#include <chrono>
#include <string>
#include <thread>

// placeholder block layout — gets replaced as real renderers are implemented
struct BlockPlaceholder {
    std::string label;
    float x, y, w, h;
};

// recomputed every frame so blocks stretch correctly on window resize
static std::array<BlockPlaceholder, 5> makePlaceholders(float W, float H)
{
    const float pad  = 12.f;
    const float colW = (W / 2.f) - pad * 1.5f;
    const float row3 = (H - pad * 4) / 3.f;
    const float row2 = (H - pad * 3) / 2.f;
    const float xL   = pad;
    const float xR   = W / 2.f + pad * 0.5f;

    return {{
        { "[ Block 1 ]  System Info",           xL, pad,            colW, row3 - pad },
        { "[ Block 2 ]  Request Log",           xL, pad*2 + row3,   colW, row3 - pad },
        { "[ Block 3 ]  External IP",           xL, pad*3 + row3*2, colW, row3 - pad },
        { "[ Block 4 ]  Network Devices",       xR, pad,            colW, row2 - pad },
        { "[ Block 5 ]  Connection Visualizer", xR, pad*2 + row2,   colW, row2 - pad },
    }};
}

static sf::Color statusColor(ToolInfo::Status s)
{
    switch (s) {
        case ToolInfo::Status::Active:       return { 120, 200, 120 };
        case ToolInfo::Status::Inactive:     return { 200, 160,  60 };
        case ToolInfo::Status::NotInstalled: return { 120, 120, 120 };
    }
    return { 255, 255, 255 };
}

static const char* statusLabel(ToolInfo::Status s)
{
    switch (s) {
        case ToolInfo::Status::Active:       return "[active]";
        case ToolInfo::Status::Inactive:     return "[inactive]";
        case ToolInfo::Status::NotInstalled: return "[not installed]";
    }
    return "";
}

ApplicationController::ApplicationController()
    : m_window(
        sf::VideoMode({ Config::WINDOW_WIDTH, Config::WINDOW_HEIGHT }),
        Config::WINDOW_TITLE
    )
{
    m_window.setFramerateLimit(Config::TARGET_FPS);

    // try common Arch/Debian paths for a monospace font
    for (auto& path : {
        "/usr/share/fonts/TTF/DejaVuSansMono.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        "/usr/share/fonts/dejavu/DejaVuSansMono.ttf"
    }) {
        if (m_font.openFromFile(path)) { m_fontLoaded = true; break; }
    }

    m_data.systemInfo = std::make_unique<SystemInfoProvider>();

    // fetch once immediately so we dont show empty block on first frame
    m_data.systemInfo->fetch();

    startDataThread();
}

ApplicationController::~ApplicationController()
{
    stopDataThread();
}

void ApplicationController::startDataThread()
{
    m_dataThread = std::thread([this] {
        while (m_running) {
            // system info doesnt change often - 30 sec refresh is plenty
            if (m_sysInfoClock.getElapsedTime().asSeconds() >= 30.f) {
                m_data.systemInfo->fetch();
                m_sysInfoClock.restart();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    });
}

void ApplicationController::stopDataThread()
{
    m_running = false;
    if (m_data.systemInfo) m_data.systemInfo->stop();
    if (m_dataThread.joinable()) m_dataThread.join();
}

void ApplicationController::run()
{
    while (m_window.isOpen()) {
        processEvents();
        update();
        render();
    }
}

void ApplicationController::processEvents()
{
    while (const auto event = m_window.pollEvent()) {
        if (event->is<sf::Event::Closed>())
            m_window.close();
        if (const auto* key = event->getIf<sf::Event::KeyPressed>())
            if (key->code == sf::Keyboard::Key::Escape)
                m_window.close();
    }
}

void ApplicationController::update()
{
    // TODO M4+: pull data from remaining providers into DataManager
}

void ApplicationController::render()
{
    m_window.clear(sf::Color(Config::BG_R, Config::BG_G, Config::BG_B));
    renderPlaceholders();
    renderSystemInfo();
    m_window.display();
}

void ApplicationController::renderPlaceholders()
{
    const float W = static_cast<float>(m_window.getSize().x);
    const float H = static_cast<float>(m_window.getSize().y);

    const sf::Color borderColor { 55, 55, 60 };
    const sf::Color labelColor  { 80, 80, 85 };
    const sf::Color waitColor   { 60, 60, 65 };

    auto blocks = makePlaceholders(W, H);

    for (size_t i = 0; i < blocks.size(); ++i) {
        auto& b = blocks[i];

        sf::RectangleShape bg({ b.w, b.h });
        bg.setPosition({ b.x, b.y });
        bg.setFillColor(sf::Color(26, 26, 30));
        bg.setOutlineThickness(1.f);
        bg.setOutlineColor(borderColor);
        m_window.draw(bg);

        if (!m_fontLoaded) continue;

        // block 1 gets a slightly brighter label since it has live data
        sf::Text lbl(m_font, b.label, 13);
        lbl.setFillColor(i == 0 ? sf::Color(100, 180, 100) : labelColor);
        lbl.setPosition({ b.x + 10.f, b.y + 8.f });
        m_window.draw(lbl);

        if (i > 0) {
            sf::Text wait(m_font, "waiting for data...", 11);
            wait.setFillColor(waitColor);
            wait.setPosition({ b.x + 10.f, b.y + 30.f });
            m_window.draw(wait);
        }
    }
}

void ApplicationController::renderSystemInfo()
{
    if (!m_fontLoaded || !m_data.systemInfo) return;

    const float W = static_cast<float>(m_window.getSize().x);
    const float H = static_cast<float>(m_window.getSize().y);
    auto blocks = makePlaceholders(W, H);
    const auto& b = blocks[0];

    auto tools = m_data.systemInfo->getData();
    if (tools.empty()) return;

    const float startY  = b.y + 30.f;
    const float lineH   = 16.f;
    const float marginX = b.x + 14.f;

    for (size_t i = 0; i < tools.size(); ++i) {
        float y = startY + static_cast<float>(i) * lineH;
        if (y + lineH > b.y + b.h) break;  // dont draw outside the block border

        auto& t = tools[i];

        sf::Text name(m_font, t.name + ": ", 11);
        name.setFillColor({ 200, 200, 200 });
        name.setPosition({ marginX, y });
        m_window.draw(name);

        sf::Text ver(m_font, t.version, 11);
        ver.setFillColor({ 160, 160, 160 });
        ver.setPosition({ marginX + 110.f, y });
        m_window.draw(ver);

        sf::Text stat(m_font, statusLabel(t.status), 11);
        stat.setFillColor(statusColor(t.status));
        stat.setPosition({ b.x + b.w - 110.f, y });
        m_window.draw(stat);
    }
}