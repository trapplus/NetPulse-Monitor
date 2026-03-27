#include "Render/ConnectionVisualizer.hpp"
#include "App/Config.hpp"
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Window/Mouse.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <unordered_map>

ConnectionVisualizer::ConnectionVisualizer() = default;
ConnectionVisualizer::~ConnectionVisualizer() = default;

void ConnectionVisualizer::setViewport(const sf::FloatRect& viewport)
{
    m_viewport = viewport;
}

void ConnectionVisualizer::setConnections(const std::vector<ConnectionInfo>& connections)
{
    m_connections = connections;
}

void ConnectionVisualizer::setFont(const sf::Font* font)
{
    m_font = font;
}

void ConnectionVisualizer::setDisplayMode(DisplayMode mode)
{
    m_displayMode = mode;
}

sf::Color ConnectionVisualizer::statusColor(ConnectionInfo::Status status)
{
    switch (status) {
        case ConnectionInfo::Status::ESTABLISHED: return Config::STATUS_ACTIVE_COLOR;
        case ConnectionInfo::Status::LISTEN: return Config::STATUS_LISTEN_COLOR;
        case ConnectionInfo::Status::TIME_WAIT: return Config::STATUS_NOT_INSTALLED_COLOR;
        case ConnectionInfo::Status::OTHER: return Config::STATUS_NOT_INSTALLED_COLOR;
    }
    return Config::STATUS_NOT_INSTALLED_COLOR;
}

const char* ConnectionVisualizer::statusLabel(ConnectionInfo::Status status)
{
    switch (status) {
        case ConnectionInfo::Status::ESTABLISHED: return "ESTABLISHED";
        case ConnectionInfo::Status::LISTEN: return "LISTEN";
        case ConnectionInfo::Status::TIME_WAIT: return "TIME_WAIT";
        case ConnectionInfo::Status::OTHER: return "OTHER";
    }
    return "OTHER";
}

sf::Color ConnectionVisualizer::headerColorForIndex(std::size_t index)
{
    return Config::CONNECTION_NODE_COLORS[index % Config::CONNECTION_NODE_COLORS.size()];
}

std::vector<ConnectionVisualizer::PeerSummary>
ConnectionVisualizer::buildPeerSummaries(const std::vector<ConnectionInfo>& connections, DisplayMode mode)
{
    std::unordered_map<std::string, PeerSummary> merged;

    for (const auto& conn : connections) {
        if (mode == DisplayMode::TCP && conn.protocol != ConnectionInfo::Protocol::TCP)
            continue;
        if (mode == DisplayMode::UDP && conn.protocol != ConnectionInfo::Protocol::UDP)
            continue;

        auto [it, inserted] = merged.try_emplace(conn.remoteIP);
        if (inserted) {
            it->second.ip = conn.remoteIP;
            it->second.status = conn.status;
        }

        it->second.weight += 1;
        if (conn.protocol == ConnectionInfo::Protocol::TCP) it->second.tcpCount += 1;
        if (conn.protocol == ConnectionInfo::Protocol::UDP) it->second.udpCount += 1;

        // if any socket to this peer is ESTABLISHED we show that as the dominant state
        if (conn.status == ConnectionInfo::Status::ESTABLISHED)
            it->second.status = ConnectionInfo::Status::ESTABLISHED;
    }

    std::vector<PeerSummary> peers;
    peers.reserve(merged.size());
    for (auto& [_, summary] : merged)
        peers.push_back(summary);

    std::sort(peers.begin(), peers.end(), [](const PeerSummary& left, const PeerSummary& right) {
        if (left.weight != right.weight)
            return left.weight > right.weight;
        return left.ip < right.ip;
    });

    return peers;
}

void ConnectionVisualizer::draw(sf::RenderWindow& window)
{
    if (m_viewport.size.x <= 0.f || m_viewport.size.y <= 0.f)
        return;

    const sf::Vector2f center {
        m_viewport.position.x + m_viewport.size.x * Config::CONNECTION_CENTER_RATIO,
        m_viewport.position.y + m_viewport.size.y * Config::CONNECTION_CENTER_RATIO
    };

    auto peers = buildPeerSummaries(m_connections, m_displayMode);
    if (peers.empty()) return;

    const float minDim = std::min(m_viewport.size.x, m_viewport.size.y);
    const float remoteRadius = std::max(Config::CONNECTION_MIN_REMOTE_RING_RADIUS,
                                        minDim * Config::CONNECTION_REMOTE_RING_RATIO);

    const std::size_t count = peers.size();
    struct PeerDrawData {
        sf::Vector2f position;
        float        radius;
        PeerSummary  summary;
    };
    std::vector<PeerDrawData> peerDrawData;
    peerDrawData.reserve(count);

    constexpr std::array<ConnectionInfo::Status, 4> stateOrder {
        ConnectionInfo::Status::ESTABLISHED,
        ConnectionInfo::Status::LISTEN,
        ConnectionInfo::Status::TIME_WAIT,
        ConnectionInfo::Status::OTHER
    };
    std::array<std::size_t, 4> stateCounts { 0, 0, 0, 0 };
    std::array<float, 4> stateSegmentStarts { 0.f, 0.f, 0.f, 0.f };
    std::array<float, 4> stateSegmentWidths { 0.f, 0.f, 0.f, 0.f };
    if (m_displayMode == DisplayMode::State) {
        for (const auto& peer : peers) {
            for (std::size_t i = 0; i < stateOrder.size(); ++i) {
                if (peer.status == stateOrder[i]) {
                    ++stateCounts[i];
                    break;
                }
            }
        }

        constexpr float fullTurn = 2.f * std::numbers::pi_v<float>;
        constexpr float baseGap = 0.12f;
        constexpr float minSegmentWidth = 0.45f;

        std::size_t activeGroups = 0;
        std::size_t totalPeers = 0;
        for (std::size_t i = 0; i < stateCounts.size(); ++i) {
            if (stateCounts[i] > 0) {
                ++activeGroups;
                totalPeers += stateCounts[i];
            }
        }

        const float totalGap = baseGap * static_cast<float>(std::max<std::size_t>(0, activeGroups));
        const float availableArc = std::max(1.f, fullTurn - totalGap);
        const float baseRequired = minSegmentWidth * static_cast<float>(activeGroups);
        const float extraArc = std::max(0.f, availableArc - baseRequired);

        float cursor = -std::numbers::pi_v<float> * 0.5f;
        for (std::size_t i = 0; i < stateCounts.size(); ++i) {
            if (stateCounts[i] == 0) {
                continue;
            }

            const float share = totalPeers > 0
                ? static_cast<float>(stateCounts[i]) / static_cast<float>(totalPeers)
                : 0.f;
            const float segmentWidth = minSegmentWidth + share * extraArc;
            stateSegmentStarts[i] = cursor;
            stateSegmentWidths[i] = segmentWidth;
            cursor += segmentWidth + baseGap;
        }
    }

    std::array<std::size_t, 4> stateOffsets { 0, 0, 0, 0 };
    for (std::size_t i = 0; i < count; ++i) {
        float angle = 0.f;
        if (m_displayMode == DisplayMode::State) {
            std::size_t groupIndex = stateOrder.size() - 1;
            for (std::size_t stateIndex = 0; stateIndex < stateOrder.size(); ++stateIndex) {
                if (peers[i].status == stateOrder[stateIndex]) {
                    groupIndex = stateIndex;
                    break;
                }
            }

            const float segmentStart = stateSegmentStarts[groupIndex];
            const float segmentWidth = stateSegmentWidths[groupIndex];
            const float usableWidth = std::max(0.18f, segmentWidth - 0.08f);
            const std::size_t localCount = std::max<std::size_t>(1, stateCounts[groupIndex]);
            const std::size_t localIndex = stateOffsets[groupIndex]++;

            if (localCount == 1) {
                angle = segmentStart + segmentWidth * 0.5f;
            } else {
                const float t = static_cast<float>(localIndex) / static_cast<float>(localCount - 1);
                angle = segmentStart + 0.04f + t * usableWidth;
            }
        } else {
            angle = (2.f * std::numbers::pi_v<float> * static_cast<float>(i))
                  / static_cast<float>(count);
        }

        const sf::Vector2f remote {
            center.x + std::cos(angle) * remoteRadius,
            center.y + std::sin(angle) * remoteRadius
        };

        sf::Vector2f delta { remote.x - center.x, remote.y - center.y };
        const float length = std::sqrt(delta.x * delta.x + delta.y * delta.y);
        if (length <= Config::CONNECTION_MIN_EDGE_LENGTH) continue;

        const float rotation = std::atan2(delta.y, delta.x)
                             * Config::CONNECTION_DEGREES_PER_RADIAN
                             / std::numbers::pi_v<float>;

        const float thickness = std::min(
            Config::CONNECTION_EDGE_MAX_THICKNESS,
            Config::CONNECTION_EDGE_BASE_THICKNESS +
            static_cast<float>(peers[i].weight) * Config::CONNECTION_EDGE_WEIGHT_SCALE
        );

        sf::RectangleShape edge({ length, thickness });
        edge.setPosition(center);
        edge.setRotation(sf::degrees(rotation));
        edge.setFillColor(statusColor(peers[i].status));
        window.draw(edge);

        const float nodeRadius = std::min(
            Config::CONNECTION_NODE_MAX_RADIUS,
            Config::CONNECTION_NODE_BASE_RADIUS +
            static_cast<float>(peers[i].weight) * Config::CONNECTION_NODE_WEIGHT_SCALE
        );
        sf::CircleShape remoteNode(nodeRadius);
        remoteNode.setOrigin({ nodeRadius, nodeRadius });
        remoteNode.setPosition(remote);
        remoteNode.setFillColor(headerColorForIndex(i));
        window.draw(remoteNode);

        peerDrawData.push_back({ remote, nodeRadius + Config::CONNECTION_HIT_RADIUS_PADDING, peers[i] });
    }

    sf::CircleShape localNode(Config::CONNECTION_LOCAL_NODE_RADIUS);
    localNode.setOrigin({ Config::CONNECTION_LOCAL_NODE_RADIUS, Config::CONNECTION_LOCAL_NODE_RADIUS });
    localNode.setPosition(center);
    localNode.setFillColor(Config::CONNECTION_LOCAL_NODE_COLOR);
    window.draw(localNode);

    if (m_font) {
        if (m_displayMode == DisplayMode::State) {
            for (std::size_t i = 0; i < stateOrder.size(); ++i) {
                if (stateCounts[i] == 0)
                    continue;

                const float angle = stateSegmentStarts[i] + stateSegmentWidths[i] * 0.5f;
                const sf::Vector2f labelPos {
                    center.x + std::cos(angle) * (remoteRadius + Config::CONNECTION_GROUP_LABEL_OFFSET_Y),
                    center.y + std::sin(angle) * (remoteRadius + Config::CONNECTION_GROUP_LABEL_OFFSET_Y)
                };

                sf::Text label(*m_font, statusLabel(stateOrder[i]), Config::BODY_FONT_SIZE);
                const sf::FloatRect labelBounds = label.getLocalBounds();
                label.setOrigin({
                    labelBounds.position.x + labelBounds.size.x * 0.5f,
                    labelBounds.position.y + labelBounds.size.y * 0.5f
                });
                label.setPosition(labelPos);
                label.setFillColor(statusColor(stateOrder[i]));
                window.draw(label);
            }
        }

        const sf::Vector2i mousePixels = sf::Mouse::getPosition(window);
        const sf::Vector2f mouse {
            static_cast<float>(mousePixels.x),
            static_cast<float>(mousePixels.y)
        };

        const PeerDrawData* hovered = nullptr;
        for (const auto& peer : peerDrawData) {
            const float dx = mouse.x - peer.position.x;
            const float dy = mouse.y - peer.position.y;
            if ((dx * dx + dy * dy) <= (peer.radius * peer.radius)) {
                hovered = &peer;
                break;
            }
        }

        if (hovered) {
            // quick tooltip near hovered peer so you can inspect endpoint details without clicks
            const std::string tooltipText =
                hovered->summary.ip + "\n" +
                "state: " + statusLabel(hovered->summary.status) + "\n" +
                "tcp: " + std::to_string(hovered->summary.tcpCount) + "  udp: " + std::to_string(hovered->summary.udpCount) + "\n" +
                "sockets: " + std::to_string(hovered->summary.weight);

            sf::Text text(*m_font, tooltipText, Config::BODY_FONT_SIZE);
            text.setFillColor(Config::CONNECTION_LOCAL_NODE_COLOR);
            text.setPosition({
                hovered->position.x + Config::CONNECTION_TOOLTIP_OFFSET_X,
                hovered->position.y + Config::CONNECTION_TOOLTIP_OFFSET_Y
            });

            const sf::FloatRect bounds = text.getGlobalBounds();
            sf::RectangleShape box({
                bounds.size.x + Config::CONNECTION_TOOLTIP_PADDING_X,
                bounds.size.y + Config::CONNECTION_TOOLTIP_PADDING_Y
            });
            box.setPosition({
                bounds.position.x - Config::CONNECTION_TOOLTIP_PADDING_X * 0.5f,
                bounds.position.y - Config::CONNECTION_TOOLTIP_PADDING_Y * 0.5f
            });
            box.setFillColor(Config::TOOLTIP_FILL_COLOR);
            box.setOutlineThickness(Config::PANEL_OUTLINE_THICKNESS);
            box.setOutlineColor(Config::TOOLTIP_OUTLINE_COLOR);

            window.draw(box);
            window.draw(text);
        }
    }

    // TODO M8: add moving particles along edges for traffic activity animation.
    // TODO M8: add mouse picking so clicking a remote node opens connection details.
}
