#include "App/ApplicationController.hpp"
#include "App/Config.hpp"
#include "Data/ConnectionProvider.hpp" // explicit - DataManager.hpp only forward-declares providers
#include "Data/ExternalAPIProvider.hpp"
#include "Data/NetworkDeviceProvider.hpp"
#include "Data/PacketSnifferProvider.hpp"
#include "Data/RequestEntry.hpp"
#include "Data/SystemInfoProvider.hpp"
#include "Render/ConnectionVisualizer.hpp"
#include <array>
#include <chrono>
#include <cstddef>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace {

enum class PanelId : std::size_t {
    SystemInfo = 0,
    RequestLog,
    ExternalIP,
    NetworkDevices,
    ConnectionVisualizer,
    Count
};

struct PanelLayout {
    std::string_view title;
    float            x;
    float            y;
    float            w;
    float            h;
};

using PanelLayoutArray = std::array<PanelLayout, static_cast<std::size_t>(PanelId::Count)>;

PanelLayoutArray makePanels(float width, float height)
{
    const float pad = Config::PANEL_PADDING;
    const float colW = (width / 2.f) - pad * 1.5f;
    const float row3 = (height - pad * 4.f) / 3.f;
    const float row2 = (height - pad * 3.f) / 2.f;
    const float xL = pad;
    const float xR = width / 2.f + pad * 0.5f;

    return {{
        { "System Info", xL, pad, colW, row3 - pad },
        { "Request Log", xL, pad * 2.f + row3, colW, row3 - pad },
        { "External IP", xL, pad * 3.f + row3 * 2.f, colW, row3 - pad },
        { "Network Devices", xR, pad, colW, row2 - pad },
        { "Connection Visualizer", xR, pad * 2.f + row2, colW, row2 - pad },
    }};
}

const PanelLayout& panelFor(const PanelLayoutArray& panels, PanelId id)
{
    return panels[static_cast<std::size_t>(id)];
}

sf::Color toolStatusColor(ToolInfo::Status status)
{
    switch (status) {
        case ToolInfo::Status::Active: return Config::STATUS_ACTIVE_COLOR;
        case ToolInfo::Status::Inactive: return Config::STATUS_INACTIVE_COLOR;
        case ToolInfo::Status::NotInstalled: return Config::STATUS_NOT_INSTALLED_COLOR;
    }
    return Config::TEXT_PRIMARY_COLOR;
}

const char* toolStatusLabel(ToolInfo::Status status)
{
    switch (status) {
        case ToolInfo::Status::Active: return "[active]";
        case ToolInfo::Status::Inactive: return "[inactive]";
        case ToolInfo::Status::NotInstalled: return "[not installed]";
    }
    return "";
}

sf::Color panelTitleColor(std::size_t index)
{
    return (index < Config::PANEL_TITLE_COLORS.size())
        ? Config::PANEL_TITLE_COLORS[index]
        : Config::TEXT_DIM_COLOR;
}

bool isCompleteDevice(const std::string& status)
{
    return status == "Complete";
}

std::string truncateText(const std::string& value, std::size_t maxLen)
{
    // Request-log columns use this helper to keep rows aligned; for tiny limits we skip ellipsis to avoid underflow.
    if (value.size() <= maxLen) {
        return value;
    }

    if (maxLen <= 3) {
        return value.substr(0, maxLen);
    }

    return value.substr(0, maxLen - 3) + "...";
}

std::string formatTimestamp(const std::chrono::system_clock::time_point& point)
{
    const std::time_t timeValue = std::chrono::system_clock::to_time_t(point);
    std::tm localTime {};
#if defined(_WIN32)
    localtime_s(&localTime, &timeValue);
#else
    // localtime_r is thread-safe, so formatting here stays safe even while worker threads keep running.
    localtime_r(&timeValue, &localTime);
#endif

    std::ostringstream stream;
    stream << std::put_time(&localTime, "%H:%M:%S");
    return stream.str();
}

sf::Color requestMethodColor(const RequestEntry& entry)
{
    // Keeping method colors centralized makes the row renderer simpler and keeps badge colors consistent.
    if (entry.isEncrypted) {
        return Config::TEXT_DIM_COLOR;
    }

    if (entry.method == "GET") {
        return Config::STATUS_ACTIVE_COLOR;
    }
    if (entry.method == "POST") {
        return Config::REQUEST_LOG_POST_COLOR;
    }
    if (entry.method == "PUT") {
        return Config::STATUS_INACTIVE_COLOR;
    }
    if (entry.method == "DELETE") {
        return Config::REQUEST_LOG_DELETE_COLOR;
    }
    if (entry.method == "PATCH") {
        return Config::REQUEST_LOG_PATCH_COLOR;
    }
    if (entry.method == "HEAD" || entry.method == "OPTIONS") {
        return Config::REQUEST_LOG_OPTIONS_COLOR;
    }

    return Config::TEXT_DIM_COLOR;
}

} // namespace

ApplicationController::ApplicationController()
    : m_window(
        sf::VideoMode({ Config::WINDOW_WIDTH, Config::WINDOW_HEIGHT }),
        Config::WINDOW_TITLE,
        sf::Style::Default
    )
{
    m_window.setFramerateLimit(Config::TARGET_FPS);

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

    // local providers are cheap enough to prime synchronously before the first frame
    m_data.systemInfo->fetch();
    m_data.networkDevices->fetch();
    m_data.connections->fetch();

    startWorkers();
}

ApplicationController::~ApplicationController()
{
    stopWorkers();
}

void ApplicationController::startWorkers()
{
    m_dataThread = std::thread([this] {
        using clock = std::chrono::steady_clock;

        auto nextSystemInfoFetch = clock::now() + Config::SYSTEM_INFO_REFRESH_INTERVAL;
        auto nextNetworkFetch = clock::now() + Config::DATA_REFRESH_INTERVAL;

        while (m_running) {
            const auto now = clock::now();

            if (now >= nextSystemInfoFetch) {
                if (m_data.systemInfo)
                    m_data.systemInfo->fetch();
                nextSystemInfoFetch = now + Config::SYSTEM_INFO_REFRESH_INTERVAL;
            }

            if (now >= nextNetworkFetch) {
                if (m_data.networkDevices)
                    m_data.networkDevices->fetch();
                if (m_data.connections)
                    m_data.connections->fetch();
                if (m_data.packetSniffer)
                    m_data.packetSniffer->fetch();
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
                if (m_data.externalAPI)
                    m_data.externalAPI->fetch();
                nextApiFetch = clock::now() + Config::API_REFRESH_INTERVAL;
            }

            std::this_thread::sleep_for(Config::WORKER_SLEEP_INTERVAL);
        }
    });
}

void ApplicationController::stopWorkers()
{
    m_running = false;

    if (m_data.systemInfo) {
        m_data.systemInfo->stop();
    }
    if (m_data.networkDevices) {
        m_data.networkDevices->stop();
    }
    if (m_data.connections) {
        m_data.connections->stop();
    }
    if (m_data.externalAPI) {
        m_data.externalAPI->stop();
    }
    if (m_data.packetSniffer) {
        m_data.packetSniffer->stop();
    }

    if (m_dataThread.joinable()) {
        m_dataThread.join();
    }
    if (m_apiThread.joinable()) {
        m_apiThread.join();
    }
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

        if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
            if (key->code == sf::Keyboard::Key::Escape) {
                m_window.close();
            }
        }

        if (const auto* click = event->getIf<sf::Event::MouseButtonPressed>()) {
            if (click->button == sf::Mouse::Button::Left) {
                const sf::Vector2f pos {
                    static_cast<float>(click->position.x),
                    static_cast<float>(click->position.y)
                };
                for (std::size_t i = 0; i < m_ifaceButtonBounds.size(); ++i) {
                    if (m_ifaceButtonBounds[i].contains(pos)) {
                        m_selectedInterface = i;
                        if (m_data.packetSniffer && i < m_interfaces.size())
                            m_data.packetSniffer->setInterface(m_interfaces[i]);
                        break;
                    }
                }
            }
        }
    }
}

void ApplicationController::update()
{
}

void ApplicationController::render()
{
    m_window.clear(sf::Color(Config::BG_R, Config::BG_G, Config::BG_B));
    renderPlaceholders();
    renderSystemInfo();
    renderRequestLog();
    renderNetworkDevices();
    renderExternalAPI();
    renderConnections();
    m_window.display();
}

void ApplicationController::renderPlaceholders()
{
    const float width = static_cast<float>(m_window.getSize().x);
    const float height = static_cast<float>(m_window.getSize().y);
    const auto panels = makePanels(width, height);

    const auto externalData = m_data.externalAPI ? m_data.externalAPI->getData() : ExternalAPIProvider::Snapshot{};
    const bool hasRequestLog = m_data.packetSniffer && !m_data.packetSniffer->getData().empty();
    const bool hasNetworkDevices = m_data.networkDevices && !m_data.networkDevices->getData().empty();
    const bool hasConnections = m_data.connections && !m_data.connections->getData().empty();

    for (std::size_t i = 0; i < panels.size(); ++i) {
        const auto& panel = panels[i];

        sf::RectangleShape bg({ panel.w, panel.h });
        bg.setPosition({ panel.x, panel.y });
        bg.setFillColor(Config::PANEL_FILL_COLOR);
        bg.setOutlineThickness(Config::PANEL_OUTLINE_THICKNESS);
        bg.setOutlineColor(Config::PANEL_BORDER_COLOR);
        m_window.draw(bg);

        if (!m_fontLoaded) {
            continue;
        }

        sf::Text title(m_font, std::string(panel.title), Config::TITLE_FONT_SIZE);
        title.setFillColor(panelTitleColor(i));
        title.setPosition({ panel.x + Config::PANEL_TITLE_OFFSET_X, panel.y + Config::PANEL_TITLE_OFFSET_Y });
        m_window.draw(title);

        if (i > 0) {
            if (i == static_cast<std::size_t>(PanelId::RequestLog) && hasRequestLog) {
                continue;
            }
            if (i == static_cast<std::size_t>(PanelId::ExternalIP) && externalData.isFresh) {
                continue;
            }
            if (i == static_cast<std::size_t>(PanelId::NetworkDevices) && hasNetworkDevices) {
                continue;
            }
            if (i == static_cast<std::size_t>(PanelId::ConnectionVisualizer) && hasConnections) {
                continue;
            }

            sf::Text wait(m_font, "waiting for data...", Config::BODY_FONT_SIZE);
            wait.setFillColor(Config::PANEL_WAIT_COLOR);
            wait.setPosition({ panel.x + Config::PANEL_TITLE_OFFSET_X, panel.y + Config::PANEL_CONTENT_TOP });
            m_window.draw(wait);
        }
    }
}

void ApplicationController::renderSystemInfo()
{
    if (!m_fontLoaded || !m_data.systemInfo) {
        return;
    }

    const auto panels = makePanels(
        static_cast<float>(m_window.getSize().x),
        static_cast<float>(m_window.getSize().y)
    );
    const auto& panel = panelFor(panels, PanelId::SystemInfo);

    const auto tools = m_data.systemInfo->getData();
    if (tools.empty()) {
        return;
    }

    const float startY = panel.y + Config::PANEL_CONTENT_TOP;
    const float lineH = Config::PANEL_LINE_HEIGHT;
    const float marginX = panel.x + Config::PANEL_INNER_PADDING;

    for (std::size_t i = 0; i < tools.size(); ++i) {
        const float y = startY + static_cast<float>(i) * lineH;
        if (y + lineH > panel.y + panel.h) {
            break;
        }

        const auto& tool = tools[i];

        sf::Text name(m_font, tool.name + ": ", Config::BODY_FONT_SIZE);
        name.setFillColor(Config::TEXT_PRIMARY_COLOR);
        name.setPosition({ marginX, y });
        m_window.draw(name);

        sf::Text version(m_font, tool.version, Config::BODY_FONT_SIZE);
        version.setFillColor(Config::TEXT_SECONDARY_COLOR);
        version.setPosition({ marginX + Config::SYSTEM_INFO_VERSION_OFFSET, y });
        m_window.draw(version);

        sf::Text status(m_font, toolStatusLabel(tool.status), Config::BODY_FONT_SIZE);
        status.setFillColor(toolStatusColor(tool.status));
        status.setPosition({ panel.x + panel.w - Config::SYSTEM_INFO_STATUS_OFFSET, y });
        m_window.draw(status);
    }
}

void ApplicationController::renderRequestLog()
{
    if (!m_fontLoaded) {
        return;
    }

    const auto panels = makePanels(
        static_cast<float>(m_window.getSize().x),
        static_cast<float>(m_window.getSize().y)
    );
    const auto& panel = panelFor(panels, PanelId::RequestLog);

    // Click targets are frame-dependent, so clear and rebuild them every render pass.
    m_ifaceButtonBounds.clear();

    const float marginX = panel.x + Config::PANEL_INNER_PADDING;
    const float titleY = panel.y + Config::PANEL_TITLE_OFFSET_Y;
    float rightEdge = panel.x + panel.w - Config::PANEL_INNER_PADDING;

    // Keep hit boxes indexed by m_interfaces, but lay out right-to-left so long interface names consume right edge first.
    m_ifaceButtonBounds.resize(m_interfaces.size());
    for (std::size_t reverse = 0; reverse < m_interfaces.size(); ++reverse) {
        const std::size_t i = m_interfaces.size() - 1 - reverse;
        sf::Text label(m_font, m_interfaces[i], Config::BODY_FONT_SIZE);
        // Local bounds gives glyph size only; global bounds would include transforms and skew button sizing.
        const sf::FloatRect textBounds = label.getLocalBounds();

        const float buttonWidth = textBounds.size.x + Config::IFACE_BUTTON_PADDING_X * 2.f;
        const float buttonHeight = textBounds.size.y + Config::IFACE_BUTTON_PADDING_Y * 2.f;
        const float buttonX = rightEdge - buttonWidth;
        const float buttonY = titleY;

        m_ifaceButtonBounds[i] = sf::FloatRect({ buttonX, buttonY }, { buttonWidth, buttonHeight });

        sf::RectangleShape button({ buttonWidth, buttonHeight });
        button.setPosition({ buttonX, buttonY });
        if (i == m_selectedInterface) {
            button.setFillColor(Config::STATUS_ACTIVE_COLOR);
        } else {
            button.setFillColor(Config::PANEL_FILL_COLOR);
            button.setOutlineColor(Config::PANEL_BORDER_COLOR);
            button.setOutlineThickness(Config::PANEL_OUTLINE_THICKNESS);
        }
        m_window.draw(button);

        label.setPosition({
            buttonX + Config::IFACE_BUTTON_PADDING_X - textBounds.position.x,
            buttonY + Config::IFACE_BUTTON_PADDING_Y - textBounds.position.y
        });
        label.setFillColor(i == m_selectedInterface ? sf::Color { 20, 20, 20 } : Config::TEXT_SECONDARY_COLOR);
        m_window.draw(label);

        rightEdge = buttonX - Config::IFACE_BUTTON_SPACING;
    }

    if (m_interfaces.empty()) {
        sf::Text noInterfaces(m_font, "no interfaces", Config::BODY_FONT_SIZE);
        noInterfaces.setFillColor(Config::TEXT_DIM_COLOR);
        noInterfaces.setPosition({ rightEdge - noInterfaces.getLocalBounds().size.x, titleY });
        m_window.draw(noInterfaces);
    }

    const float startY = panel.y + Config::PANEL_CONTENT_TOP + Config::PANEL_LINE_HEIGHT;
    const float lineH = Config::PANEL_LINE_HEIGHT;

    const auto entries = m_data.packetSniffer
        ? m_data.packetSniffer->getData()
        : std::vector<RequestEntry> {};

    if (entries.empty() && m_data.packetSniffer) {
        sf::Text empty(m_font, "no traffic captured", Config::BODY_FONT_SIZE);
        empty.setFillColor(Config::TEXT_DIM_COLOR);
        empty.setPosition({ marginX, startY });
        m_window.draw(empty);
        return;
    }

    std::size_t row = 0;
    // Show newest rows first so fresh traffic is immediately visible without scrolling.
    for (auto it = entries.rbegin(); it != entries.rend(); ++it) {
        const float y = startY + static_cast<float>(row) * lineH;
        if (y + lineH > panel.y + panel.h) {
            break;
        }

        const auto& entry = *it;

        sf::Text timeText(m_font, formatTimestamp(entry.timestamp), Config::BODY_FONT_SIZE);
        timeText.setFillColor(Config::TEXT_DIM_COLOR);
        timeText.setPosition({ marginX, y });
        m_window.draw(timeText);

        const std::string methodLabel = entry.isEncrypted ? "HTTPS" : entry.method;
        sf::Text methodText(m_font, methodLabel, Config::BODY_FONT_SIZE);
        methodText.setFillColor(requestMethodColor(entry));
        methodText.setPosition({ marginX + Config::REQUEST_LOG_TIME_OFFSET, y });
        m_window.draw(methodText);

        sf::Text hostText(m_font, truncateText(entry.host, 28), Config::BODY_FONT_SIZE);
        hostText.setFillColor(Config::TEXT_PRIMARY_COLOR);
        hostText.setPosition({ marginX + Config::REQUEST_LOG_TIME_OFFSET + Config::REQUEST_LOG_METHOD_OFFSET, y });
        m_window.draw(hostText);

        const std::string tailText = entry.isEncrypted
            ? std::to_string(entry.payloadBytes) + " bytes"
            : truncateText(entry.path, 35);
        sf::Text tail(m_font, tailText, Config::BODY_FONT_SIZE);
        tail.setFillColor(entry.isEncrypted ? Config::TEXT_DIM_COLOR : Config::TEXT_SECONDARY_COLOR);
        tail.setPosition({
            marginX + Config::REQUEST_LOG_TIME_OFFSET + Config::REQUEST_LOG_METHOD_OFFSET + Config::REQUEST_LOG_HOST_OFFSET,
            y
        });
        m_window.draw(tail);

        ++row;
    }
}

void ApplicationController::renderNetworkDevices()
{
    if (!m_fontLoaded || !m_data.networkDevices) {
        return;
    }

    const auto panels = makePanels(
        static_cast<float>(m_window.getSize().x),
        static_cast<float>(m_window.getSize().y)
    );
    const auto& panel = panelFor(panels, PanelId::NetworkDevices);

    const auto devices = m_data.networkDevices->getData();
    if (devices.empty()) {
        return;
    }

    const float startY = panel.y + Config::PANEL_CONTENT_TOP;
    const float lineH = Config::PANEL_LINE_HEIGHT;
    const float marginX = panel.x + Config::PANEL_INNER_PADDING;

    for (std::size_t i = 0; i < devices.size(); ++i) {
        const float y = startY + static_cast<float>(i) * lineH;
        if (y + lineH > panel.y + panel.h) {
            break;
        }

        const auto& device = devices[i];

        sf::Text ip(m_font, device.ip, Config::BODY_FONT_SIZE);
        ip.setFillColor(Config::TEXT_PRIMARY_COLOR);
        ip.setPosition({ marginX, y });
        m_window.draw(ip);

        sf::Text mac(m_font, device.mac, Config::BODY_FONT_SIZE);
        mac.setFillColor(Config::TEXT_SECONDARY_COLOR);
        mac.setPosition({ marginX + Config::NETWORK_MAC_OFFSET, y });
        m_window.draw(mac);

        sf::Text iface(m_font, device.iface, Config::BODY_FONT_SIZE);
        iface.setFillColor(Config::INTERFACE_ACCENT_COLOR);
        iface.setPosition({ marginX + Config::NETWORK_IFACE_OFFSET, y });
        m_window.draw(iface);

        sf::Text status(m_font, device.status, Config::BODY_FONT_SIZE);
        status.setFillColor(isCompleteDevice(device.status) ? Config::STATUS_ACTIVE_COLOR
                                                            : Config::STATUS_INACTIVE_COLOR);
        status.setPosition({ panel.x + panel.w - Config::NETWORK_STATUS_OFFSET, y });
        m_window.draw(status);
    }
}

void ApplicationController::renderExternalAPI()
{
    if (!m_fontLoaded || !m_data.externalAPI) {
        return;
    }

    const auto snapshot = m_data.externalAPI->getData();
    if (!snapshot.isFresh) {
        return;
    }

    const auto panels = makePanels(
        static_cast<float>(m_window.getSize().x),
        static_cast<float>(m_window.getSize().y)
    );
    const auto& panel = panelFor(panels, PanelId::ExternalIP);

    const float startY = panel.y + Config::EXTERNAL_API_CONTENT_TOP;
    const float lineH = Config::PANEL_LINE_HEIGHT;
    const float marginX = panel.x + Config::PANEL_INNER_PADDING;

    sf::Text ip(m_font, "IP: " + snapshot.ip, Config::BODY_FONT_SIZE);
    ip.setFillColor(Config::EXTERNAL_IP_COLOR);
    ip.setPosition({ marginX, startY });
    m_window.draw(ip);

    sf::Text provider(m_font, "Provider: " + snapshot.provider, Config::BODY_FONT_SIZE);
    provider.setFillColor(Config::EXTERNAL_PROVIDER_COLOR);
    provider.setPosition({ marginX, startY + lineH });
    m_window.draw(provider);

    sf::Text city(m_font, "City: " + snapshot.city, Config::BODY_FONT_SIZE);
    city.setFillColor(Config::EXTERNAL_LOCATION_COLOR);
    city.setPosition({ marginX, startY + lineH * 2.f });
    m_window.draw(city);

    sf::Text country(m_font, "Country: " + snapshot.country, Config::BODY_FONT_SIZE);
    country.setFillColor(Config::STATUS_ACTIVE_COLOR);
    country.setPosition({ marginX, startY + lineH * 3.f });
    m_window.draw(country);
}

void ApplicationController::renderConnections()
{
    if (!m_data.connections) {
        return;
    }

    const auto panels = makePanels(
        static_cast<float>(m_window.getSize().x),
        static_cast<float>(m_window.getSize().y)
    );
    const auto& panel = panelFor(panels, PanelId::ConnectionVisualizer);
    const auto connections = m_data.connections->getData();

    static ConnectionVisualizer visualizer;
    visualizer.setViewport({
        { panel.x + Config::PANEL_VISUALIZER_SIDE_PADDING, panel.y + Config::PANEL_VISUALIZER_TOP },
        { panel.w - Config::PANEL_VISUALIZER_SIDE_PADDING * 2.f, panel.h - Config::PANEL_VISUALIZER_BOTTOM }
    });
    visualizer.setConnections(connections);
    visualizer.setFont(m_fontLoaded ? &m_font : nullptr);
    visualizer.draw(m_window);

    if (!m_fontLoaded) {
        return;
    }

    int tcpCount = 0;
    int udpCount = 0;
    for (const auto& connection : connections) {
        if (connection.protocol == ConnectionInfo::Protocol::TCP) {
            ++tcpCount;
        } else if (connection.protocol == ConnectionInfo::Protocol::UDP) {
            ++udpCount;
        }
    }

    const float statsY = panel.y + Config::CONNECTION_STATS_TOP;

    sf::Text tcp(m_font, "TCP: " + std::to_string(tcpCount), Config::BODY_FONT_SIZE);
    tcp.setFillColor(Config::CONNECTION_TCP_COLOR);
    tcp.setPosition({ panel.x + Config::CONNECTION_STATS_OFFSET_X, statsY });
    m_window.draw(tcp);

    sf::Text udp(m_font, "UDP: " + std::to_string(udpCount), Config::BODY_FONT_SIZE);
    udp.setFillColor(Config::CONNECTION_UDP_COLOR);
    udp.setPosition({ panel.x + Config::CONNECTION_STATS_OFFSET_X + Config::CONNECTION_STAT_X, statsY });
    m_window.draw(udp);

    sf::Text total(m_font, "TOTAL: " + std::to_string(tcpCount + udpCount), Config::BODY_FONT_SIZE);
    total.setFillColor(Config::CONNECTION_TOTAL_COLOR);
    total.setPosition({ panel.x + Config::CONNECTION_STATS_OFFSET_X + Config::CONNECTION_STAT_X * 2.f, statsY });
    m_window.draw(total);
}
