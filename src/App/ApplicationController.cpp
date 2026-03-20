#include "App/ApplicationController.hpp"
#include "App/Config.hpp"
#include <array>
#include <string>

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
        if (m_font.openFromFile(path)) {
            m_fontLoaded = true;
            break;
        }
    }
}

ApplicationController::~ApplicationController() = default;

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
    // SFML3: события через pollEvent возвращают std::optional
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
    // TODO M3+: синхронизация данных из DataManager -> рендереры
}

void ApplicationController::render()
{
    m_window.clear(sf::Color(Config::BG_R, Config::BG_G, Config::BG_B));
    renderPlaceholders();
    m_window.display();
}

void ApplicationController::renderPlaceholders()
{
    const float W = static_cast<float>(m_window.getSize().x);
    const float H = static_cast<float>(m_window.getSize().y);

    const sf::Color borderColor { 55,  55,  60  };
    const sf::Color labelColor  { 120, 200, 120 };
    const sf::Color statusColor { 60,  60,  65  };

    for (auto& b : makePlaceholders(W, H)) {
        sf::RectangleShape bg({ b.w, b.h });
        bg.setPosition({ b.x, b.y });
        bg.setFillColor(sf::Color(26, 26, 30));
        bg.setOutlineThickness(1.f);
        bg.setOutlineColor(borderColor);
        m_window.draw(bg);

        if (!m_fontLoaded) continue;

        sf::Text label(m_font, b.label, 14);
        label.setFillColor(labelColor);
        label.setPosition({ b.x + 10.f, b.y + 10.f });
        m_window.draw(label);

        sf::Text status(m_font, "waiting for data...", 11);
        status.setFillColor(statusColor);
        status.setPosition({ b.x + 10.f, b.y + 32.f });
        m_window.draw(status);
    }
}
