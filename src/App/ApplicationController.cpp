#include "App/ApplicationController.hpp"
#include "App/Config.hpp"
#include <array>
#include <string>
#include <thread>
#include <chrono>

// ── Заглушки блоков (убираются по мере реализации) ───────────────────────────
struct BlockPlaceholder {
    std::string label;
    float x, y, w, h;
};

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

// ── Цвета статусов ────────────────────────────────────────────────────────────
static sf::Color statusColor(ToolInfo::Status s)
{
    switch (s) {
        case ToolInfo::Status::Active:       return { 120, 200, 120 };  // зелёный
        case ToolInfo::Status::Inactive:     return { 200, 160,  60 };  // жёлтый
        case ToolInfo::Status::NotInstalled: return { 120, 120, 120 };  // серый
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

// ── Конструктор ───────────────────────────────────────────────────────────────
ApplicationController::ApplicationController()
    : m_window(
        sf::VideoMode({ Config::WINDOW_WIDTH, Config::WINDOW_HEIGHT }),
        Config::WINDOW_TITLE
    )
{
    m_window.setFramerateLimit(Config::TARGET_FPS);

    for (auto& path : {
        "/usr/share/fonts/TTF/DejaVuSansMono.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        "/usr/share/fonts/dejavu/DejaVuSansMono.ttf"
    }) {
        if (m_font.openFromFile(path)) { m_fontLoaded = true; break; }
    }

    // Инициализируем провайдеры
    m_data.systemInfo = std::make_unique<SystemInfoProvider>();

    // Первый fetch сразу — чтобы не ждать 30 сек
    m_data.systemInfo->fetch();

    startDataThread();
}

ApplicationController::~ApplicationController()
{
    stopDataThread();
}

// ── Data thread ───────────────────────────────────────────────────────────────
void ApplicationController::startDataThread()
{
    m_dataThread = std::thread([this] {
        while (m_running) {
            // Обновляем System Info каждые 30 секунд
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

// ── Main loop ─────────────────────────────────────────────────────────────────
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
    // TODO M4+: обновление остальных провайдеров
}

void ApplicationController::render()
{
    m_window.clear(sf::Color(Config::BG_R, Config::BG_G, Config::BG_B));

    renderPlaceholders();  // рисует рамки всех блоков
    renderSystemInfo();    // рисует данные в блоке 1

    m_window.display();
}

// ── Заглушки блоков (рамки) ───────────────────────────────────────────────────
void ApplicationController::renderPlaceholders()
{
    const float W = static_cast<float>(m_window.getSize().x);
    const float H = static_cast<float>(m_window.getSize().y);

    const sf::Color borderColor { 55, 55, 60 };
    const sf::Color labelColor  { 80, 80, 85 };   // приглушённый — блок 1 теперь с данными
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

        // Заголовок блока (для блока 1 — тоже рисуем, renderSystemInfo его не перекрывает)
        sf::Text lbl(m_font, b.label, 13);
        lbl.setFillColor(i == 0 ? sf::Color(100, 180, 100) : labelColor);
        lbl.setPosition({ b.x + 10.f, b.y + 8.f });
        m_window.draw(lbl);

        // "waiting for data..." только для блоков 2-5
        if (i > 0) {
            sf::Text wait(m_font, "waiting for data...", 11);
            wait.setFillColor(waitColor);
            wait.setPosition({ b.x + 10.f, b.y + 30.f });
            m_window.draw(wait);
        }
    }
}

// ── Рендер блока 1: System Info ───────────────────────────────────────────────
void ApplicationController::renderSystemInfo()
{
    if (!m_fontLoaded || !m_data.systemInfo) return;

    const float W = static_cast<float>(m_window.getSize().x);
    const float H = static_cast<float>(m_window.getSize().y);
    auto blocks   = makePlaceholders(W, H);
    auto& b       = blocks[0];   // блок 1 — левый верхний

    auto tools = m_data.systemInfo->getData();
    if (tools.empty()) return;

    const float startY   = b.y + 30.f;   // ниже заголовка
    const float lineH    = 16.f;
    const float marginX  = b.x + 14.f;

    for (size_t i = 0; i < tools.size(); ++i) {
        float y = startY + static_cast<float>(i) * lineH;
        if (y + lineH > b.y + b.h) break;   // не выходим за рамку

        auto& t = tools[i];

        // Имя утилиты
        sf::Text name(m_font, t.name + ": ", 11);
        name.setFillColor({ 200, 200, 200 });
        name.setPosition({ marginX, y });
        m_window.draw(name);

        // Версия
        float versionX = marginX + 110.f;
        sf::Text ver(m_font, t.version, 11);
        ver.setFillColor({ 160, 160, 160 });
        ver.setPosition({ versionX, y });
        m_window.draw(ver);

        // Статус с цветом
        float statusX = b.x + b.w - 110.f;
        sf::Text stat(m_font, statusLabel(t.status), 11);
        stat.setFillColor(statusColor(t.status));
        stat.setPosition({ statusX, y });
        m_window.draw(stat);
    }
}
