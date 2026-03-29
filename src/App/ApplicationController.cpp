#include "App/ApplicationController.hpp"
#include "App/Config.hpp"
#include "Data/ConnectionProvider.hpp"
#include "Data/ExternalAPIProvider.hpp"
#include "Data/NetworkDeviceProvider.hpp"
#include "Data/PacketSnifferProvider.hpp"
#include "Data/RequestEntry.hpp"
#include "Data/SystemInfoProvider.hpp"
#include "Render/BlockRenderers.hpp"
#include "Render/PanelLayout.hpp"
#include "Render/UIDrawUtils.hpp"
#include <chrono>
#include <optional>
#include <thread>
#include <vector>

ApplicationController::ApplicationController()
    : m_window(sf::VideoMode({ Config::WINDOW_WIDTH, Config::WINDOW_HEIGHT }), Config::WINDOW_TITLE, sf::Style::Default)
{
    m_window.setFramerateLimit(Config::TARGET_FPS);
    m_window.setMinimumSize(std::optional<sf::Vector2u> { sf::Vector2u { Config::MIN_WINDOW_WIDTH, Config::MIN_WINDOW_HEIGHT } });

    for (const char* path : Config::FONT_PATHS) {
        if (m_font.openFromFile(path)) {
            m_fontLoaded = true;
            break;
        }
    }

    m_data.systemInfo = std::make_unique<SystemInfoProvider>();
    m_data.networkDevices = std::make_unique<NetworkDeviceProvider>();
    m_data.connections = std::make_unique<ConnectionProvider>();
    m_data.externalAPI = std::make_unique<ExternalAPIProvider>();
    m_data.packetSniffer = std::make_unique<PacketSnifferProvider>();
    m_interfaces = m_data.packetSniffer->getAvailableInterfaces();

    m_data.systemInfo->fetch();
    m_data.networkDevices->fetch();
    m_data.connections->fetch();
    startWorkers();
}

ApplicationController::~ApplicationController() { stopWorkers(); }

void ApplicationController::startWorkers()
{
    // Split workers keep API latency spikes away from the tighter local-data polling cadence.
    m_dataThread = std::thread([this] {
        using clock = std::chrono::steady_clock;
        auto nextSystemInfoFetch = clock::now() + Config::SYSTEM_INFO_REFRESH_INTERVAL;
        auto nextNetworkFetch = clock::now() + Config::DATA_REFRESH_INTERVAL;

        while (m_running) {
            const auto now = clock::now();
            if (now >= nextSystemInfoFetch) {
                if (m_data.systemInfo) m_data.systemInfo->fetch();
                nextSystemInfoFetch = now + Config::SYSTEM_INFO_REFRESH_INTERVAL;
            }
            if (now >= nextNetworkFetch) {
                if (m_data.networkDevices) m_data.networkDevices->fetch();
                if (m_data.connections) m_data.connections->fetch();
                if (m_data.packetSniffer) m_data.packetSniffer->fetch();
                nextNetworkFetch = now + Config::DATA_REFRESH_INTERVAL;
            }
            std::this_thread::sleep_for(Config::WORKER_SLEEP_INTERVAL);
        }
    });

    m_apiThread = std::thread([this] {
        using clock = std::chrono::steady_clock;
        auto nextApiFetch = clock::time_point::min();

        while (m_running) {
            const auto now = clock::now();
            if (now >= nextApiFetch) {
                if (m_data.externalAPI) m_data.externalAPI->fetch();
                nextApiFetch = clock::now() + Config::API_REFRESH_INTERVAL;
            }
            std::this_thread::sleep_for(Config::WORKER_SLEEP_INTERVAL);
        }
    });
}

void ApplicationController::stopWorkers()
{
    // Flip the shared flag first so both loops exit naturally before joining.
    m_running = false;
    if (m_data.systemInfo) m_data.systemInfo->stop();
    if (m_data.networkDevices) m_data.networkDevices->stop();
    if (m_data.connections) m_data.connections->stop();
    if (m_data.externalAPI) m_data.externalAPI->stop();
    if (m_data.packetSniffer) m_data.packetSniffer->stop();
    if (m_dataThread.joinable()) m_dataThread.join();
    if (m_apiThread.joinable()) m_apiThread.join();
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
        if (event->is<sf::Event::Closed>()) {
            m_window.close();
            continue;
        }
        if (const auto* key = event->getIf<sf::Event::KeyPressed>(); key && key->code == sf::Keyboard::Key::Escape) {
            m_window.close();
        }
        if (const auto* resized = event->getIf<sf::Event::Resized>()) {
            m_window.setView(sf::View(
                sf::Vector2f { static_cast<float>(resized->size.x) * 0.5f, static_cast<float>(resized->size.y) * 0.5f },
                sf::Vector2f { static_cast<float>(resized->size.x), static_cast<float>(resized->size.y) }
            ));
        }

        const auto* click = event->getIf<sf::Event::MouseButtonPressed>();
        if (!click || click->button != sf::Mouse::Button::Left) {
            continue;
        }

        const sf::Vector2f pos { static_cast<float>(click->position.x), static_cast<float>(click->position.y) };
        bool handled = false;
        // Interface buttons win click priority because changing capture source is an explicit action.
        for (std::size_t i = 0; i < m_ifaceButtonBounds.size(); ++i) {
            if (!m_ifaceButtonBounds[i].contains(pos)) continue;
            m_selectedInterface = i;
            if (m_data.packetSniffer && i < m_interfaces.size()) m_data.packetSniffer->setInterface(m_interfaces[i]);
            handled = true;
            break;
        }

        if (!handled) {
            // Visualizer mode acts like a fallback toolbar if no interface selector consumed the click.
            for (std::size_t i = 0; i < m_visualizerModeButtonBounds.size(); ++i) {
                if (m_visualizerModeButtonBounds[i].contains(pos)) {
                    m_selectedVisualizerMode = i;
                    break;
                }
            }
        }
    }
}

void ApplicationController::update() {}

void ApplicationController::render()
{
    // Coordinator decides *when* to draw each block; BlockRender owns *how* each block is painted.
    m_window.clear(sf::Color(Config::BG_R, Config::BG_G, Config::BG_B));
    renderPlaceholders();

    const auto panels = Panel::makePanels(static_cast<float>(m_window.getSize().x), static_cast<float>(m_window.getSize().y));

    if (m_fontLoaded && m_data.systemInfo) {
        BlockRender::systemInfo(m_window, m_font, Panel::panelFor(panels, Panel::PanelId::SystemInfo), m_data.systemInfo->getData());
    }

    if (m_fontLoaded) {
        BlockRender::requestLog(
            m_window,
            m_font,
            Panel::panelFor(panels, Panel::PanelId::RequestLog),
            m_interfaces,
            m_selectedInterface,
            m_ifaceButtonBounds,
            m_data.packetSniffer ? m_data.packetSniffer->getData() : std::vector<RequestEntry> {}
        );
    }

    if (m_fontLoaded && m_data.networkDevices) {
        BlockRender::networkDevices(m_window, m_font, Panel::panelFor(panels, Panel::PanelId::NetworkDevices), m_data.networkDevices->getData());
    }
    if (m_fontLoaded && m_data.externalAPI) {
        BlockRender::externalAPI(m_window, m_font, Panel::panelFor(panels, Panel::PanelId::ExternalIP), m_data.externalAPI->getData());
    }
    if (m_data.connections) {
        BlockRender::connections(
            m_window,
            m_fontLoaded ? &m_font : nullptr,
            Panel::panelFor(panels, Panel::PanelId::ConnectionVisualizer),
            m_data.connections->getData(),
            m_selectedVisualizerMode,
            m_visualizerModeButtonBounds,
            m_visualizer
        );
    }

    m_window.display();
}

void ApplicationController::renderPlaceholders()
{
    const auto panels = Panel::makePanels(static_cast<float>(m_window.getSize().x), static_cast<float>(m_window.getSize().y));
    const auto externalData = m_data.externalAPI ? m_data.externalAPI->getData() : ExternalAPIProvider::Snapshot{};
    const bool hasRequestLog = m_data.packetSniffer && !m_data.packetSniffer->getData().empty();
    const bool hasNetworkDevices = m_data.networkDevices && !m_data.networkDevices->getData().empty();
    const bool hasConnections = m_data.connections && !m_data.connections->getData().empty();

    for (std::size_t i = 0; i < panels.size(); ++i) {
        const auto& panel = panels[i];
        Draw::drawRoundedFrame(m_window, sf::FloatRect { { panel.x, panel.y }, { panel.w, panel.h } });
        if (!m_fontLoaded) continue;

        Draw::drawPanelHeader(m_window, m_font, panel, static_cast<Panel::PanelId>(i));
        if (i == 0) continue;

        // Keep placeholders only for blocks that are still empty so loaded panels look immediately live.
        if (i == static_cast<std::size_t>(Panel::PanelId::RequestLog) && hasRequestLog) continue;
        if (i == static_cast<std::size_t>(Panel::PanelId::ExternalIP) && externalData.isFresh) continue;
        if (i == static_cast<std::size_t>(Panel::PanelId::NetworkDevices) && hasNetworkDevices) continue;
        if (i == static_cast<std::size_t>(Panel::PanelId::ConnectionVisualizer) && hasConnections) continue;

        sf::Text wait(m_font, "waiting for data...", Config::BODY_FONT_SIZE);
        wait.setFillColor(Config::PANEL_WAIT_COLOR);
        wait.setPosition({ panel.x + Config::PANEL_TITLE_OFFSET_X, panel.y + Config::PANEL_CONTENT_TOP });
        m_window.draw(wait);
    }
}
