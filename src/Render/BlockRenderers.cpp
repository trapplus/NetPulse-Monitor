#include "Render/BlockRenderers.hpp"
#include "App/Config.hpp"
#include "Render/UIDrawUtils.hpp"
#include "Render/UIFormatUtils.hpp"
#include <algorithm>
#include <array>
#include <string>
#include <unordered_set>

namespace BlockRender {

void systemInfo(sf::RenderWindow& window,
                const sf::Font& font,
                const Panel::PanelLayout& panel,
                const std::vector<ToolInfo>& tools)
{
    // If provider has no snapshot yet we leave placeholder text visible from coordinator.
    if (tools.empty()) {
        return;
    }

    const sf::FloatRect surface = Draw::makeContentSurfaceRect(panel, Config::PANEL_CONTENT_TOP);
    Draw::drawContentSurface(window, surface);

    const float marginX = surface.position.x + Config::CONTENT_SURFACE_PADDING_X;
    const float headerY = surface.position.y + Config::CONTENT_SURFACE_PADDING_Y;
    const float startY = headerY + Config::PANEL_TABLE_HEADER_HEIGHT;
    const float lineH = Config::PANEL_LINE_HEIGHT;

    sf::Text nameHeader(font, "service", Config::CAPTION_FONT_SIZE);
    nameHeader.setFillColor(Config::TEXT_DIM_COLOR);
    nameHeader.setPosition({ marginX, headerY });
    window.draw(nameHeader);

    sf::Text versionHeader(font, "version", Config::CAPTION_FONT_SIZE);
    versionHeader.setFillColor(Config::TEXT_DIM_COLOR);
    versionHeader.setPosition({ marginX + Config::SYSTEM_INFO_VERSION_OFFSET, headerY });
    window.draw(versionHeader);

    sf::Text statusHeader(font, "status", Config::CAPTION_FONT_SIZE);
    statusHeader.setFillColor(Config::TEXT_DIM_COLOR);
    statusHeader.setPosition({ surface.position.x + surface.size.x - Config::SYSTEM_INFO_STATUS_OFFSET, headerY });
    window.draw(statusHeader);

    for (std::size_t i = 0; i < tools.size(); ++i) {
        const float y = startY + static_cast<float>(i) * lineH;
        if (y + lineH > surface.position.y + surface.size.y - Config::CONTENT_SURFACE_PADDING_Y) {
            break;
        }

        const auto& tool = tools[i];

        sf::Text name(font, tool.name + ": ", Config::BODY_FONT_SIZE);
        name.setFillColor(Config::TEXT_PRIMARY_COLOR);
        name.setPosition({ marginX, y });
        window.draw(name);

        sf::Text version(font, tool.version, Config::BODY_FONT_SIZE);
        version.setFillColor(Config::TEXT_SECONDARY_COLOR);
        version.setPosition({ marginX + Config::SYSTEM_INFO_VERSION_OFFSET, y });
        window.draw(version);

        sf::Text status(font, Fmt::toolStatusLabel(tool.status), Config::BODY_FONT_SIZE);
        status.setFillColor(Fmt::toolStatusColor(tool.status));
        status.setPosition({ surface.position.x + surface.size.x - Config::SYSTEM_INFO_STATUS_OFFSET, y });
        window.draw(status);
    }
}

void requestLog(sf::RenderWindow& window,
                const sf::Font& font,
                const Panel::PanelLayout& panel,
                const std::vector<std::string>& interfaces,
                std::size_t selectedInterface,
                std::vector<sf::FloatRect>& ifaceButtonBoundsOut,
                const std::vector<RequestEntry>& entries)
{
    // Bounds are rebuilt every frame because they depend on current panel width and selected font metrics.
    ifaceButtonBoundsOut.clear();

    const float titleY = panel.y + Config::PANEL_TITLE_OFFSET_Y;
    float rightEdge = panel.x + panel.w - Config::PANEL_INNER_PADDING;

    ifaceButtonBoundsOut.resize(interfaces.size());
    for (std::size_t reverse = 0; reverse < interfaces.size(); ++reverse) {
        const std::size_t i = interfaces.size() - 1 - reverse;
        sf::Text label(font, interfaces[i], Config::BODY_FONT_SIZE);
        const sf::FloatRect textBounds = label.getLocalBounds();

        const float buttonWidth = textBounds.size.x + Config::IFACE_BUTTON_PADDING_X * 2.f;
        const float buttonHeight = textBounds.size.y + Config::IFACE_BUTTON_PADDING_Y * 2.f;
        const float buttonX = rightEdge - buttonWidth;
        const float buttonY = titleY;

        ifaceButtonBoundsOut[i] = sf::FloatRect({ buttonX, buttonY }, { buttonWidth, buttonHeight });
        rightEdge = buttonX - Config::IFACE_BUTTON_SPACING;
    }

    for (std::size_t i = 0; i < interfaces.size(); ++i) {
        const auto& bounds = ifaceButtonBoundsOut[i];
        Draw::drawPillButton(window, bounds, i == selectedInterface);

        sf::Text label(font, interfaces[i], Config::BODY_FONT_SIZE);
        const sf::FloatRect textBounds = label.getLocalBounds();
        label.setPosition({
            bounds.position.x + Config::IFACE_BUTTON_PADDING_X - textBounds.position.x,
            bounds.position.y + Config::IFACE_BUTTON_PADDING_Y - textBounds.position.y
        });
        label.setFillColor(i == selectedInterface ? sf::Color { 20, 20, 20 } : Config::TEXT_SECONDARY_COLOR);
        window.draw(label);
    }

    if (interfaces.empty()) {
        sf::Text noInterfaces(font, "no interfaces", Config::BODY_FONT_SIZE);
        noInterfaces.setFillColor(Config::TEXT_DIM_COLOR);
        noInterfaces.setPosition({ rightEdge - noInterfaces.getLocalBounds().size.x, titleY });
        window.draw(noInterfaces);
    }

    const sf::FloatRect surface = Draw::makeContentSurfaceRect(panel, Config::PANEL_CONTENT_TOP);
    Draw::drawContentSurface(window, surface);

    const float marginX = surface.position.x + Config::CONTENT_SURFACE_PADDING_X;
    const float headerY = surface.position.y + Config::CONTENT_SURFACE_PADDING_Y;
    const float startY = headerY + Config::PANEL_TABLE_HEADER_HEIGHT;
    const float lineH = Config::PANEL_LINE_HEIGHT;

    sf::Text timeHeader(font, "time", Config::CAPTION_FONT_SIZE);
    timeHeader.setFillColor(Config::TEXT_DIM_COLOR);
    timeHeader.setPosition({ marginX, headerY });
    window.draw(timeHeader);

    sf::Text methodHeader(font, "method", Config::CAPTION_FONT_SIZE);
    methodHeader.setFillColor(Config::TEXT_DIM_COLOR);
    methodHeader.setPosition({ marginX + Config::REQUEST_LOG_TIME_OFFSET, headerY });
    window.draw(methodHeader);

    sf::Text hostHeader(font, "host / ip", Config::CAPTION_FONT_SIZE);
    hostHeader.setFillColor(Config::TEXT_DIM_COLOR);
    hostHeader.setPosition({ marginX + Config::REQUEST_LOG_TIME_OFFSET + Config::REQUEST_LOG_METHOD_OFFSET, headerY });
    window.draw(hostHeader);

    sf::Text bytesHeader(font, "path / bytes", Config::CAPTION_FONT_SIZE);
    bytesHeader.setFillColor(Config::TEXT_DIM_COLOR);
    bytesHeader.setPosition({
        marginX + Config::REQUEST_LOG_TIME_OFFSET + Config::REQUEST_LOG_METHOD_OFFSET + Config::REQUEST_LOG_HOST_OFFSET,
        headerY
    });
    window.draw(bytesHeader);

    if (entries.empty()) {
        sf::Text empty(font, "no traffic captured", Config::BODY_FONT_SIZE);
        empty.setFillColor(Config::TEXT_DIM_COLOR);
        empty.setPosition({ marginX, startY });
        window.draw(empty);
        return;
    }

    std::size_t row = 0;
    // Newest-first ordering makes live traffic changes visible without scrolling support.
    for (auto it = entries.rbegin(); it != entries.rend(); ++it) {
        const float y = startY + static_cast<float>(row) * lineH;
        if (y + lineH > surface.position.y + surface.size.y - Config::CONTENT_SURFACE_PADDING_Y) {
            break;
        }

        const auto& entry = *it;

        sf::Text timeText(font, Fmt::formatTimestamp(entry.timestamp), Config::BODY_FONT_SIZE);
        timeText.setFillColor(Config::TEXT_DIM_COLOR);
        timeText.setPosition({ marginX, y });
        window.draw(timeText);

        const std::string methodLabel = entry.isEncrypted ? "HTTPS" : entry.method;
        sf::Text methodText(font, methodLabel, Config::BODY_FONT_SIZE);
        methodText.setFillColor(Fmt::requestMethodColor(entry));
        methodText.setPosition({ marginX + Config::REQUEST_LOG_TIME_OFFSET, y });
        window.draw(methodText);

        sf::Text hostText(font, Fmt::truncateText(entry.host, 28), Config::BODY_FONT_SIZE);
        hostText.setFillColor(Config::TEXT_PRIMARY_COLOR);
        hostText.setPosition({ marginX + Config::REQUEST_LOG_TIME_OFFSET + Config::REQUEST_LOG_METHOD_OFFSET, y });
        window.draw(hostText);

        const std::string tailText = entry.isEncrypted
            ? std::to_string(entry.payloadBytes) + " bytes"
            : Fmt::truncateText(entry.path, 35);
        sf::Text tail(font, tailText, Config::BODY_FONT_SIZE);
        tail.setFillColor(entry.isEncrypted ? Config::TEXT_DIM_COLOR : Config::TEXT_SECONDARY_COLOR);
        tail.setPosition({
            marginX + Config::REQUEST_LOG_TIME_OFFSET + Config::REQUEST_LOG_METHOD_OFFSET + Config::REQUEST_LOG_HOST_OFFSET,
            y
        });
        window.draw(tail);

        ++row;
    }
}

void networkDevices(sf::RenderWindow& window,
                    const sf::Font& font,
                    const Panel::PanelLayout& panel,
                    const std::vector<NetworkDeviceInfo>& devices)
{
    // Summary chips are cheap context, so we derive them here instead of storing redundant provider fields.
    if (devices.empty()) {
        return;
    }

    const sf::FloatRect surface = Draw::makeContentSurfaceRect(panel, Config::NETWORK_TABLE_TOP);
    Draw::drawContentSurface(window, surface);

    const float headerY = surface.position.y + Config::CONTENT_SURFACE_PADDING_Y;
    const float startY = headerY + Config::PANEL_TABLE_HEADER_HEIGHT;
    const float lineH = Config::PANEL_LINE_HEIGHT;
    const float marginX = surface.position.x + Config::CONTENT_SURFACE_PADDING_X;
    int completeCount = 0;
    std::unordered_set<std::string> interfaces;
    for (const auto& device : devices) {
        if (Fmt::isCompleteDevice(device.status)) {
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
        sf::Text label(font, item.first, Config::CAPTION_FONT_SIZE);
        const sf::FloatRect textBounds = label.getLocalBounds();
        const sf::FloatRect chipBounds(
            { summaryX, panel.y + Config::NETWORK_SUMMARY_TOP },
            {
                textBounds.size.x + Config::MODE_BUTTON_PADDING_X * 1.6f,
                textBounds.size.y + Config::MODE_BUTTON_PADDING_Y * 1.5f
            }
        );
        Draw::drawRoundedRect(window, chipBounds, Config::BUTTON_CORNER_RADIUS, Config::BUTTON_IDLE_FILL_COLOR);
        label.setFillColor(item.second);
        label.setPosition({
            chipBounds.position.x + Config::MODE_BUTTON_PADDING_X * 0.8f - textBounds.position.x,
            chipBounds.position.y + Config::MODE_BUTTON_PADDING_Y * 0.55f - textBounds.position.y
        });
        window.draw(label);
        summaryX = chipBounds.position.x + chipBounds.size.x + Config::NETWORK_SUMMARY_GAP;
    }

    // Hover lookup stays local to render because it depends on frame-accurate row geometry.
    const sf::Vector2i mousePixels = sf::Mouse::getPosition(window);
    const sf::Vector2f mouse { static_cast<float>(mousePixels.x), static_cast<float>(mousePixels.y) };
    const NetworkDeviceInfo* hoveredDevice = nullptr;

    sf::Text ipHeader(font, "ip", Config::CAPTION_FONT_SIZE);
    ipHeader.setFillColor(Config::TEXT_DIM_COLOR);
    ipHeader.setPosition({ marginX, headerY });
    window.draw(ipHeader);

    sf::Text macHeader(font, "mac", Config::CAPTION_FONT_SIZE);
    macHeader.setFillColor(Config::TEXT_DIM_COLOR);
    macHeader.setPosition({ marginX + Config::NETWORK_MAC_OFFSET, headerY });
    window.draw(macHeader);

    sf::Text ifaceHeader(font, "iface", Config::CAPTION_FONT_SIZE);
    ifaceHeader.setFillColor(Config::TEXT_DIM_COLOR);
    ifaceHeader.setPosition({ marginX + Config::NETWORK_IFACE_OFFSET, headerY });
    window.draw(ifaceHeader);

    sf::Text statusHeader(font, "status", Config::CAPTION_FONT_SIZE);
    statusHeader.setFillColor(Config::TEXT_DIM_COLOR);
    statusHeader.setPosition({ surface.position.x + surface.size.x - Config::NETWORK_STATUS_OFFSET, headerY });
    window.draw(statusHeader);

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
            Draw::drawHoverRow(window, rowBounds);
        }

        sf::Text ip(font, device.ip, Config::BODY_FONT_SIZE);
        ip.setFillColor(Config::TEXT_PRIMARY_COLOR);
        ip.setPosition({ marginX, y });
        window.draw(ip);

        sf::Text mac(font, device.mac, Config::BODY_FONT_SIZE);
        mac.setFillColor(Config::TEXT_SECONDARY_COLOR);
        mac.setPosition({ marginX + Config::NETWORK_MAC_OFFSET, y });
        window.draw(mac);

        sf::Text iface(font, device.iface, Config::BODY_FONT_SIZE);
        iface.setFillColor(Config::INTERFACE_ACCENT_COLOR);
        iface.setPosition({ marginX + Config::NETWORK_IFACE_OFFSET, y });
        window.draw(iface);

        sf::Text status(font, device.status, Config::BODY_FONT_SIZE);
        status.setFillColor(Fmt::isCompleteDevice(device.status) ? Config::STATUS_ACTIVE_COLOR
                                                                  : Config::STATUS_INACTIVE_COLOR);
        status.setPosition({ surface.position.x + surface.size.x - Config::NETWORK_STATUS_OFFSET, y });
        window.draw(status);
    }

    if (hoveredDevice) {
        sf::Text footer(
            font,
            hoveredDevice->ip + " -> " + hoveredDevice->mac + " on " + hoveredDevice->iface,
            Config::CAPTION_FONT_SIZE
        );
        footer.setFillColor(Config::TEXT_DIM_COLOR);
        footer.setPosition({
            marginX,
            surface.position.y + surface.size.y - Config::NETWORK_FOOTER_HEIGHT
        });
        window.draw(footer);
    }
}

void externalAPI(sf::RenderWindow& window,
                 const sf::Font& font,
                 const Panel::PanelLayout& panel,
                 const ExternalAPIProvider::Snapshot& snapshot)
{
    // Freshness gate avoids showing stale geo/IP values when the API thread is still warming up.
    if (!snapshot.isFresh) {
        return;
    }

    const float lineH = Config::EXTERNAL_API_LINE_GAP;
    const sf::FloatRect surface = Draw::makeContentSurfaceRect(panel, Config::EXTERNAL_API_CONTENT_TOP);
    Draw::drawContentSurface(window, surface);
    const float externalMarginX = surface.position.x + Config::CONTENT_SURFACE_PADDING_X;
    const float contentTop = surface.position.y + Config::CONTENT_SURFACE_PADDING_Y;

    sf::Text ip(font, "IP: " + snapshot.ip, Config::BODY_FONT_SIZE);
    ip.setFillColor(Config::EXTERNAL_IP_COLOR);
    ip.setPosition({ externalMarginX, contentTop });
    window.draw(ip);

    sf::Text provider(font, "Provider: " + snapshot.provider, Config::BODY_FONT_SIZE);
    provider.setFillColor(Config::EXTERNAL_PROVIDER_COLOR);
    provider.setPosition({ externalMarginX, contentTop + lineH });
    window.draw(provider);

    sf::Text city(font, "City: " + snapshot.city, Config::BODY_FONT_SIZE);
    city.setFillColor(Config::EXTERNAL_LOCATION_COLOR);
    city.setPosition({ externalMarginX, contentTop + lineH * 2.f });
    window.draw(city);

    sf::Text country(font, "Country: " + snapshot.country, Config::BODY_FONT_SIZE);
    country.setFillColor(Config::STATUS_ACTIVE_COLOR);
    country.setPosition({ externalMarginX, contentTop + lineH * 3.f });
    window.draw(country);
}

void connections(sf::RenderWindow& window,
                 const sf::Font* font,
                 const Panel::PanelLayout& panel,
                 const std::vector<ConnectionInfo>& connections,
                 std::size_t selectedMode,
                 std::vector<sf::FloatRect>& modeButtonBoundsOut,
                 ConnectionVisualizer& visualizer)
{
    // Mode buttons live here so hit boxes and labels stay in sync with the visualizer viewport.
    modeButtonBoundsOut.clear();

    if (font != nullptr) {
        float rightEdge = panel.x + panel.w - Config::PANEL_INNER_PADDING;
        const float buttonY = panel.y + Config::PANEL_TITLE_OFFSET_Y - 1.f;
        modeButtonBoundsOut.resize(Panel::kVisualizerModeLabels.size());

        for (std::size_t reverse = 0; reverse < Panel::kVisualizerModeLabels.size(); ++reverse) {
            const std::size_t i = Panel::kVisualizerModeLabels.size() - 1 - reverse;
            sf::Text label(*font, std::string(Panel::kVisualizerModeLabels[i]), Config::BODY_FONT_SIZE);
            const sf::FloatRect textBounds = label.getLocalBounds();
            const float buttonWidth = textBounds.size.x + Config::MODE_BUTTON_PADDING_X * 2.f;
            const float buttonHeight = textBounds.size.y + Config::MODE_BUTTON_PADDING_Y * 2.f;
            const float buttonX = rightEdge - buttonWidth;

            modeButtonBoundsOut[i] = sf::FloatRect(
                { buttonX, buttonY },
                { buttonWidth, buttonHeight }
            );
            rightEdge = buttonX - Config::MODE_BUTTON_SPACING;
        }

        for (std::size_t i = 0; i < Panel::kVisualizerModeLabels.size(); ++i) {
            const auto& bounds = modeButtonBoundsOut[i];
            Draw::drawPillButton(window, bounds, i == selectedMode);

            sf::Text label(*font, std::string(Panel::kVisualizerModeLabels[i]), Config::BODY_FONT_SIZE);
            const sf::FloatRect textBounds = label.getLocalBounds();
            label.setPosition({
                bounds.position.x + Config::MODE_BUTTON_PADDING_X - textBounds.position.x,
                bounds.position.y + Config::MODE_BUTTON_PADDING_Y - textBounds.position.y
            });
            label.setFillColor(i == selectedMode ? sf::Color { 20, 20, 20 } : Config::TEXT_SECONDARY_COLOR);
            window.draw(label);
        }
    }

    visualizer.setViewport({
        { panel.x + Config::PANEL_VISUALIZER_SIDE_PADDING, panel.y + Config::PANEL_VISUALIZER_TOP },
        { panel.w - Config::PANEL_VISUALIZER_SIDE_PADDING * 2.f, panel.h - Config::PANEL_VISUALIZER_BOTTOM }
    });
    visualizer.setConnections(connections);
    visualizer.setFont(font);
    visualizer.setDisplayMode(static_cast<ConnectionVisualizer::DisplayMode>(std::min(
        selectedMode,
        Panel::kVisualizerModeLabels.size() - 1
    )));
    visualizer.draw(window);
}

} // namespace BlockRender
