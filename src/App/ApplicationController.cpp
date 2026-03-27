#include "App/ApplicationController.hpp"
#include "App/Config.hpp"
#include "Data/ConnectionProvider.hpp" // explicit - DataManager.hpp only forward-declares providers
#include "Data/ExternalAPIProvider.hpp"
#include "Data/NetworkDeviceProvider.hpp"
#include "Data/PacketSnifferProvider.hpp"
#include "Data/RequestEntry.hpp"
#include "Data/SystemInfoProvider.hpp"
#include "Render/ConnectionVisualizer.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <ctime>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_set>
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
using LabelArray = std::array<std::string_view, 4>;

constexpr LabelArray kVisualizerModeLabels { "ALL", "TCP", "UDP", "STATE" };

std::string_view panelKicker(PanelId id)
{
    switch (id) {
        case PanelId::SystemInfo: return "RUNTIME";
        case PanelId::RequestLog: return "TRAFFIC";
        case PanelId::ExternalIP: return "UPLINK";
        case PanelId::NetworkDevices: return "LAN";
        case PanelId::ConnectionVisualizer: return "GRAPH";
        case PanelId::Count: break;
    }
    return "";
}

PanelLayoutArray makePanels(float width, float height)
{
    width = std::max(width, static_cast<float>(Config::MIN_WINDOW_WIDTH));
    height = std::max(height, static_cast<float>(Config::MIN_WINDOW_HEIGHT));

    const float pad = Config::PANEL_PADDING;
    const float colW = (width - pad * 3.f) * 0.5f;
    const float contentH = height - pad * 2.f;
    const float leftTopH = Config::SYSTEM_INFO_FIXED_HEIGHT;
    const float leftBottomH = contentH - pad - leftTopH;
    const float rightTopH = Config::NETWORK_DEVICES_FIXED_HEIGHT;
    const float rightMidH = Config::EXTERNAL_API_FIXED_HEIGHT;
    const float rightBottomH = contentH - pad * 2.f - rightTopH - rightMidH;
    const float xL = pad;
    const float xR = xL + colW + pad;

    return {{
        { "System Info", xL, pad, colW, leftTopH },
        { "Request Log", xL, pad + leftTopH + pad, colW, leftBottomH },
        { "External IP", xR, pad + rightTopH + pad, colW, rightMidH },
        { "Network Devices", xR, pad, colW, rightTopH },
        { "Connection Visualizer", xR, pad + rightTopH + pad + rightMidH + pad, colW, rightBottomH },
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

bool isCompleteDevice(const std::string& status)
{
    return status == "Complete";
}

void drawRoundedRect(sf::RenderWindow& window,
                     const sf::FloatRect& rect,
                     float radius,
                     const sf::Color& fillColor)
{
    const float clampedRadius = std::min(radius, std::min(rect.size.x, rect.size.y) * 0.5f);
    if (clampedRadius <= 0.f) {
        sf::RectangleShape box(rect.size);
        box.setPosition(rect.position);
        box.setFillColor(fillColor);
        window.draw(box);
        return;
    }

    sf::RectangleShape horizontal({ rect.size.x - clampedRadius * 2.f, rect.size.y });
    horizontal.setPosition({ rect.position.x + clampedRadius, rect.position.y });
    horizontal.setFillColor(fillColor);
    window.draw(horizontal);

    sf::RectangleShape vertical({ rect.size.x, rect.size.y - clampedRadius * 2.f });
    vertical.setPosition({ rect.position.x, rect.position.y + clampedRadius });
    vertical.setFillColor(fillColor);
    window.draw(vertical);

    for (const sf::Vector2f center : {
             sf::Vector2f { rect.position.x + clampedRadius, rect.position.y + clampedRadius },
             sf::Vector2f { rect.position.x + rect.size.x - clampedRadius, rect.position.y + clampedRadius },
             sf::Vector2f { rect.position.x + clampedRadius, rect.position.y + rect.size.y - clampedRadius },
             sf::Vector2f { rect.position.x + rect.size.x - clampedRadius, rect.position.y + rect.size.y - clampedRadius },
         }) {
        sf::CircleShape corner(clampedRadius, 24);
        corner.setOrigin({ clampedRadius, clampedRadius });
        corner.setPosition(center);
        corner.setFillColor(fillColor);
        window.draw(corner);
    }
}

void drawRoundedFrame(sf::RenderWindow& window, const sf::FloatRect& rect)
{
    const sf::FloatRect shadowRect {
        { rect.position.x, rect.position.y + 2.f },
        rect.size
    };
    drawRoundedRect(window, shadowRect, Config::PANEL_CORNER_RADIUS, Config::PANEL_SHADOW_COLOR);
    drawRoundedRect(window, rect, Config::PANEL_CORNER_RADIUS, Config::PANEL_BORDER_COLOR);

    const float inset = Config::PANEL_OUTLINE_THICKNESS;
    drawRoundedRect(window,
                    sf::FloatRect {
                        { rect.position.x + inset, rect.position.y + inset },
                        { rect.size.x - inset * 2.f, rect.size.y - inset * 2.f }
                    },
                    std::max(0.f, Config::PANEL_CORNER_RADIUS - inset),
                    Config::PANEL_FILL_COLOR);
}

void drawPillButton(sf::RenderWindow& window,
                    const sf::FloatRect& bounds,
                    bool selected)
{
    drawRoundedRect(window,
                    bounds,
                    Config::BUTTON_CORNER_RADIUS,
                    selected ? Config::STATUS_ACTIVE_COLOR : Config::BUTTON_IDLE_FILL_COLOR);
}

void drawContentSurface(sf::RenderWindow& window, const sf::FloatRect& bounds)
{
    drawRoundedRect(window,
                    bounds,
                    Config::BUTTON_CORNER_RADIUS + 2.f,
                    Config::CONTENT_SURFACE_OUTLINE_COLOR);
    drawRoundedRect(window,
                    sf::FloatRect {
                        { bounds.position.x + 1.f, bounds.position.y + 1.f },
                        { bounds.size.x - 2.f, bounds.size.y - 2.f }
                    },
                    std::max(0.f, Config::BUTTON_CORNER_RADIUS + 1.f),
                    Config::CONTENT_SURFACE_FILL_COLOR);
}

void drawHoverRow(sf::RenderWindow& window, const sf::FloatRect& bounds)
{
    drawRoundedRect(window,
                    bounds,
                    Config::BUTTON_CORNER_RADIUS,
                    Config::ROW_HOVER_FILL_COLOR);
}

sf::FloatRect makeContentSurfaceRect(const PanelLayout& panel, float contentTop)
{
    return sf::FloatRect {
        { panel.x + Config::CONTENT_SURFACE_INSET_X, panel.y + contentTop + Config::CONTENT_SURFACE_TOP_OFFSET },
        {
            panel.w - Config::CONTENT_SURFACE_INSET_X * 2.f,
            panel.h - contentTop - Config::CONTENT_SURFACE_BOTTOM_INSET
        }
    };
}

void drawPanelHeader(sf::RenderWindow& window,
                     const sf::Font& titleFont,
                     const PanelLayout& panel,
                     PanelId id)
{
    const float textX = panel.x + Config::PANEL_TITLE_OFFSET_X;

    sf::Text kicker(titleFont, std::string(panelKicker(id)), Config::KICKER_FONT_SIZE);
    kicker.setFillColor(Config::TEXT_DIM_COLOR);
    kicker.setLetterSpacing(1.3f);
    kicker.setPosition({ textX, panel.y + Config::PANEL_TITLE_OFFSET_Y + Config::HEADER_KICKER_OFFSET_Y });
    window.draw(kicker);

    sf::Text title(titleFont, std::string(panel.title), Config::TITLE_FONT_SIZE);
    title.setFillColor(Config::HEADER_ICON_FILL_COLOR);
    title.setPosition({ textX, panel.y + Config::PANEL_TITLE_OFFSET_Y + Config::HEADER_TITLE_OFFSET_Y });
    window.draw(title);
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
    m_window.setMinimumSize(std::optional<sf::Vector2u> {
        sf::Vector2u { Config::MIN_WINDOW_WIDTH, Config::MIN_WINDOW_HEIGHT }
    });

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

        if (const auto* resized = event->getIf<sf::Event::Resized>()) {
            m_window.setView(sf::View(
                sf::Vector2f {
                    static_cast<float>(resized->size.x) * 0.5f,
                    static_cast<float>(resized->size.y) * 0.5f
                },
                sf::Vector2f {
                    static_cast<float>(resized->size.x),
                    static_cast<float>(resized->size.y)
                }
            ));
        }

        if (const auto* click = event->getIf<sf::Event::MouseButtonPressed>()) {
            if (click->button == sf::Mouse::Button::Left) {
                const sf::Vector2f pos {
                    static_cast<float>(click->position.x),
                    static_cast<float>(click->position.y)
                };
                bool handled = false;

                for (std::size_t i = 0; i < m_ifaceButtonBounds.size(); ++i) {
                    if (m_ifaceButtonBounds[i].contains(pos)) {
                        m_selectedInterface = i;
                        if (m_data.packetSniffer && i < m_interfaces.size())
                            m_data.packetSniffer->setInterface(m_interfaces[i]);
                        handled = true;
                        break;
                    }
                }

                if (!handled) {
                    for (std::size_t i = 0; i < m_visualizerModeButtonBounds.size(); ++i) {
                        if (m_visualizerModeButtonBounds[i].contains(pos)) {
                            m_selectedVisualizerMode = i;
                            break;
                        }
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

        drawRoundedFrame(m_window, sf::FloatRect { { panel.x, panel.y }, { panel.w, panel.h } });

        if (!m_fontLoaded) {
            continue;
        }

        drawPanelHeader(
            m_window,
            m_font,
            panel,
            static_cast<PanelId>(i)
        );

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

    const sf::FloatRect surface = makeContentSurfaceRect(panel, Config::PANEL_CONTENT_TOP);
    drawContentSurface(m_window, surface);

    const float marginX = surface.position.x + Config::CONTENT_SURFACE_PADDING_X;
    const float headerY = surface.position.y + Config::CONTENT_SURFACE_PADDING_Y;
    const float startY = headerY + Config::PANEL_TABLE_HEADER_HEIGHT;
    const float lineH = Config::PANEL_LINE_HEIGHT;

    sf::Text nameHeader(m_font, "service", Config::CAPTION_FONT_SIZE);
    nameHeader.setFillColor(Config::TEXT_DIM_COLOR);
    nameHeader.setPosition({ marginX, headerY });
    m_window.draw(nameHeader);

    sf::Text versionHeader(m_font, "version", Config::CAPTION_FONT_SIZE);
    versionHeader.setFillColor(Config::TEXT_DIM_COLOR);
    versionHeader.setPosition({ marginX + Config::SYSTEM_INFO_VERSION_OFFSET, headerY });
    m_window.draw(versionHeader);

    sf::Text statusHeader(m_font, "status", Config::CAPTION_FONT_SIZE);
    statusHeader.setFillColor(Config::TEXT_DIM_COLOR);
    statusHeader.setPosition({ surface.position.x + surface.size.x - Config::SYSTEM_INFO_STATUS_OFFSET, headerY });
    m_window.draw(statusHeader);

    for (std::size_t i = 0; i < tools.size(); ++i) {
        const float y = startY + static_cast<float>(i) * lineH;
        if (y + lineH > surface.position.y + surface.size.y - Config::CONTENT_SURFACE_PADDING_Y) {
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
        status.setPosition({ surface.position.x + surface.size.x - Config::SYSTEM_INFO_STATUS_OFFSET, y });
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
        rightEdge = buttonX - Config::IFACE_BUTTON_SPACING;
    }

    if (!m_interfaces.empty()) {
        for (std::size_t i = 0; i < m_interfaces.size(); ++i) {
            const auto& bounds = m_ifaceButtonBounds[i];
            drawPillButton(m_window, bounds, i == m_selectedInterface);

            sf::Text label(m_font, m_interfaces[i], Config::BODY_FONT_SIZE);
            const sf::FloatRect textBounds = label.getLocalBounds();
            label.setPosition({
                bounds.position.x + Config::IFACE_BUTTON_PADDING_X - textBounds.position.x,
                bounds.position.y + Config::IFACE_BUTTON_PADDING_Y - textBounds.position.y
            });
            label.setFillColor(i == m_selectedInterface ? sf::Color { 20, 20, 20 } : Config::TEXT_SECONDARY_COLOR);
            m_window.draw(label);
        }
    }

    if (m_interfaces.empty()) {
        sf::Text noInterfaces(m_font, "no interfaces", Config::BODY_FONT_SIZE);
        noInterfaces.setFillColor(Config::TEXT_DIM_COLOR);
        noInterfaces.setPosition({ rightEdge - noInterfaces.getLocalBounds().size.x, titleY });
        m_window.draw(noInterfaces);
    }

    const sf::FloatRect surface = makeContentSurfaceRect(panel, Config::PANEL_CONTENT_TOP);
    drawContentSurface(m_window, surface);

    const float marginX = surface.position.x + Config::CONTENT_SURFACE_PADDING_X;
    const float headerY = surface.position.y + Config::CONTENT_SURFACE_PADDING_Y;
    const float startY = headerY + Config::PANEL_TABLE_HEADER_HEIGHT;
    const float lineH = Config::PANEL_LINE_HEIGHT;

    sf::Text timeHeader(m_font, "time", Config::CAPTION_FONT_SIZE);
    timeHeader.setFillColor(Config::TEXT_DIM_COLOR);
    timeHeader.setPosition({ marginX, headerY });
    m_window.draw(timeHeader);

    sf::Text methodHeader(m_font, "method", Config::CAPTION_FONT_SIZE);
    methodHeader.setFillColor(Config::TEXT_DIM_COLOR);
    methodHeader.setPosition({ marginX + Config::REQUEST_LOG_TIME_OFFSET, headerY });
    m_window.draw(methodHeader);

    sf::Text hostHeader(m_font, "host / ip", Config::CAPTION_FONT_SIZE);
    hostHeader.setFillColor(Config::TEXT_DIM_COLOR);
    hostHeader.setPosition({ marginX + Config::REQUEST_LOG_TIME_OFFSET + Config::REQUEST_LOG_METHOD_OFFSET, headerY });
    m_window.draw(hostHeader);

    sf::Text bytesHeader(m_font, "path / bytes", Config::CAPTION_FONT_SIZE);
    bytesHeader.setFillColor(Config::TEXT_DIM_COLOR);
    bytesHeader.setPosition({
        marginX + Config::REQUEST_LOG_TIME_OFFSET + Config::REQUEST_LOG_METHOD_OFFSET + Config::REQUEST_LOG_HOST_OFFSET,
        headerY
    });
    m_window.draw(bytesHeader);

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
        if (y + lineH > surface.position.y + surface.size.y - Config::CONTENT_SURFACE_PADDING_Y) {
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

    const sf::FloatRect surface = makeContentSurfaceRect(panel, Config::NETWORK_TABLE_TOP);
    drawContentSurface(m_window, surface);

    const float headerY = surface.position.y + Config::CONTENT_SURFACE_PADDING_Y;
    const float startY = headerY + Config::PANEL_TABLE_HEADER_HEIGHT;
    const float lineH = Config::PANEL_LINE_HEIGHT;
    const float marginX = surface.position.x + Config::CONTENT_SURFACE_PADDING_X;
    int completeCount = 0;
    std::unordered_set<std::string> interfaces;
    for (const auto& device : devices) {
        if (isCompleteDevice(device.status)) {
            ++completeCount;
        }
        interfaces.insert(device.iface);
    }

    const std::array<std::pair<std::string, sf::Color>, 3> summary = {{
        { "hosts " + std::to_string(devices.size()), Config::TEXT_PRIMARY_COLOR },
        { "ready " + std::to_string(completeCount), Config::STATUS_ACTIVE_COLOR },
        { "ifaces " + std::to_string(interfaces.size()), Config::INTERFACE_ACCENT_COLOR }
    }};

    float summaryX = marginX;
    for (const auto& item : summary) {
        sf::Text label(m_font, item.first, Config::CAPTION_FONT_SIZE);
        const sf::FloatRect textBounds = label.getLocalBounds();
        const sf::FloatRect chipBounds(
            { summaryX, panel.y + Config::NETWORK_SUMMARY_TOP },
            {
                textBounds.size.x + Config::MODE_BUTTON_PADDING_X * 1.6f,
                textBounds.size.y + Config::MODE_BUTTON_PADDING_Y * 1.5f
            }
        );
        drawRoundedRect(m_window, chipBounds, Config::BUTTON_CORNER_RADIUS, Config::BUTTON_IDLE_FILL_COLOR);
        label.setFillColor(item.second);
        label.setPosition({
            chipBounds.position.x + Config::MODE_BUTTON_PADDING_X * 0.8f - textBounds.position.x,
            chipBounds.position.y + Config::MODE_BUTTON_PADDING_Y * 0.55f - textBounds.position.y
        });
        m_window.draw(label);
        summaryX = chipBounds.position.x + chipBounds.size.x + Config::NETWORK_SUMMARY_GAP;
    }

    const sf::Vector2i mousePixels = sf::Mouse::getPosition(m_window);
    const sf::Vector2f mouse { static_cast<float>(mousePixels.x), static_cast<float>(mousePixels.y) };
    const NetworkDeviceInfo* hoveredDevice = nullptr;

    sf::Text ipHeader(m_font, "ip", Config::CAPTION_FONT_SIZE);
    ipHeader.setFillColor(Config::TEXT_DIM_COLOR);
    ipHeader.setPosition({ marginX, headerY });
    m_window.draw(ipHeader);

    sf::Text macHeader(m_font, "mac", Config::CAPTION_FONT_SIZE);
    macHeader.setFillColor(Config::TEXT_DIM_COLOR);
    macHeader.setPosition({ marginX + Config::NETWORK_MAC_OFFSET, headerY });
    m_window.draw(macHeader);

    sf::Text ifaceHeader(m_font, "iface", Config::CAPTION_FONT_SIZE);
    ifaceHeader.setFillColor(Config::TEXT_DIM_COLOR);
    ifaceHeader.setPosition({ marginX + Config::NETWORK_IFACE_OFFSET, headerY });
    m_window.draw(ifaceHeader);

    sf::Text statusHeader(m_font, "status", Config::CAPTION_FONT_SIZE);
    statusHeader.setFillColor(Config::TEXT_DIM_COLOR);
    statusHeader.setPosition({ surface.position.x + surface.size.x - Config::NETWORK_STATUS_OFFSET, headerY });
    m_window.draw(statusHeader);

    for (std::size_t i = 0; i < devices.size(); ++i) {
        const float y = startY + static_cast<float>(i) * lineH;
        if (y + lineH > surface.position.y + surface.size.y - Config::NETWORK_FOOTER_HEIGHT) {
            break;
        }

        const auto& device = devices[i];
        const sf::FloatRect rowBounds(
            { surface.position.x + 2.f, y - 2.f },
            { surface.size.x - 4.f, lineH + 2.f }
        );
        const bool hovered = rowBounds.contains(mouse);
        if (hovered) {
            hoveredDevice = &device;
            drawHoverRow(m_window, rowBounds);
        }

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
        status.setPosition({ surface.position.x + surface.size.x - Config::NETWORK_STATUS_OFFSET, y });
        m_window.draw(status);
    }

    if (hoveredDevice) {
        sf::Text footer(
            m_font,
            hoveredDevice->ip + " -> " + hoveredDevice->mac + " on " + hoveredDevice->iface,
            Config::CAPTION_FONT_SIZE
        );
        footer.setFillColor(Config::TEXT_DIM_COLOR);
        footer.setPosition({
            marginX,
            surface.position.y + surface.size.y - Config::NETWORK_FOOTER_HEIGHT
        });
        m_window.draw(footer);
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

    const float lineH = Config::EXTERNAL_API_LINE_GAP;
    const sf::FloatRect surface = makeContentSurfaceRect(panel, Config::EXTERNAL_API_CONTENT_TOP);
    drawContentSurface(m_window, surface);
    const float externalMarginX = surface.position.x + Config::CONTENT_SURFACE_PADDING_X;
    const float contentTop = surface.position.y + Config::CONTENT_SURFACE_PADDING_Y;

    sf::Text ip(m_font, "IP: " + snapshot.ip, Config::BODY_FONT_SIZE);
    ip.setFillColor(Config::EXTERNAL_IP_COLOR);
    ip.setPosition({ externalMarginX, contentTop });
    m_window.draw(ip);

    sf::Text provider(m_font, "Provider: " + snapshot.provider, Config::BODY_FONT_SIZE);
    provider.setFillColor(Config::EXTERNAL_PROVIDER_COLOR);
    provider.setPosition({ externalMarginX, contentTop + lineH });
    m_window.draw(provider);

    sf::Text city(m_font, "City: " + snapshot.city, Config::BODY_FONT_SIZE);
    city.setFillColor(Config::EXTERNAL_LOCATION_COLOR);
    city.setPosition({ externalMarginX, contentTop + lineH * 2.f });
    m_window.draw(city);

    sf::Text country(m_font, "Country: " + snapshot.country, Config::BODY_FONT_SIZE);
    country.setFillColor(Config::STATUS_ACTIVE_COLOR);
    country.setPosition({ externalMarginX, contentTop + lineH * 3.f });
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
    m_visualizerModeButtonBounds.clear();

    if (m_fontLoaded) {
        float rightEdge = panel.x + panel.w - Config::PANEL_INNER_PADDING;
        const float buttonY = panel.y + Config::PANEL_TITLE_OFFSET_Y - 1.f;
        m_visualizerModeButtonBounds.resize(kVisualizerModeLabels.size());

        for (std::size_t reverse = 0; reverse < kVisualizerModeLabels.size(); ++reverse) {
            const std::size_t i = kVisualizerModeLabels.size() - 1 - reverse;
            sf::Text label(m_font, std::string(kVisualizerModeLabels[i]), Config::BODY_FONT_SIZE);
            const sf::FloatRect textBounds = label.getLocalBounds();
            const float buttonWidth = textBounds.size.x + Config::MODE_BUTTON_PADDING_X * 2.f;
            const float buttonHeight = textBounds.size.y + Config::MODE_BUTTON_PADDING_Y * 2.f;
            const float buttonX = rightEdge - buttonWidth;

            m_visualizerModeButtonBounds[i] = sf::FloatRect(
                { buttonX, buttonY },
                { buttonWidth, buttonHeight }
            );
            rightEdge = buttonX - Config::MODE_BUTTON_SPACING;
        }

        for (std::size_t i = 0; i < kVisualizerModeLabels.size(); ++i) {
            const auto& bounds = m_visualizerModeButtonBounds[i];
            drawPillButton(m_window, bounds, i == m_selectedVisualizerMode);

            sf::Text label(m_font, std::string(kVisualizerModeLabels[i]), Config::BODY_FONT_SIZE);
            const sf::FloatRect textBounds = label.getLocalBounds();
            label.setPosition({
                bounds.position.x + Config::MODE_BUTTON_PADDING_X - textBounds.position.x,
                bounds.position.y + Config::MODE_BUTTON_PADDING_Y - textBounds.position.y
            });
            label.setFillColor(i == m_selectedVisualizerMode ? sf::Color { 20, 20, 20 } : Config::TEXT_SECONDARY_COLOR);
            m_window.draw(label);
        }
    }

    static ConnectionVisualizer visualizer;
    visualizer.setViewport({
        { panel.x + Config::PANEL_VISUALIZER_SIDE_PADDING, panel.y + Config::PANEL_VISUALIZER_TOP },
        { panel.w - Config::PANEL_VISUALIZER_SIDE_PADDING * 2.f, panel.h - Config::PANEL_VISUALIZER_BOTTOM }
    });
    visualizer.setConnections(connections);
    visualizer.setFont(m_fontLoaded ? &m_font : nullptr);
    visualizer.setDisplayMode(static_cast<ConnectionVisualizer::DisplayMode>(std::min(
        m_selectedVisualizerMode,
        kVisualizerModeLabels.size() - 1
    )));
    visualizer.draw(m_window);

    if (!m_fontLoaded) {
        return;
    }

}
