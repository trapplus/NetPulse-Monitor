#include "Render/ConnectionVisualizer.hpp"
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <algorithm>
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

sf::Color ConnectionVisualizer::statusColor(ConnectionInfo::Status status)
{
    switch (status) {
        case ConnectionInfo::Status::ESTABLISHED: return { 120, 200, 120 };
        case ConnectionInfo::Status::LISTEN: return { 200, 180, 80 };
        case ConnectionInfo::Status::TIME_WAIT: return { 120, 120, 120 };
        case ConnectionInfo::Status::OTHER: return { 120, 120, 120 };
    }
    return { 120, 120, 120 };
}

std::vector<ConnectionVisualizer::PeerSummary>
ConnectionVisualizer::buildPeerSummaries(const std::vector<ConnectionInfo>& connections)
{
    std::unordered_map<std::string, PeerSummary> merged;

    for (const auto& conn : connections) {
        auto [it, inserted] = merged.try_emplace(conn.remoteIP);
        if (inserted) {
            it->second.ip = conn.remoteIP;
            it->second.status = conn.status;
        }

        it->second.weight += 1;

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
        m_viewport.position.x + m_viewport.size.x * 0.5f,
        m_viewport.position.y + m_viewport.size.y * 0.5f
    };

    auto peers = buildPeerSummaries(m_connections);
    if (peers.empty()) return;

    // limiting visible peers keeps the graph readable instead of turning into a solid ring
    constexpr std::size_t MAX_VISIBLE_PEERS = 22;
    if (peers.size() > MAX_VISIBLE_PEERS)
        peers.resize(MAX_VISIBLE_PEERS);

    const float minDim = std::min(m_viewport.size.x, m_viewport.size.y);
    const float remoteRadius = std::max(30.f, minDim * 0.35f);

    const std::size_t count = peers.size();
    for (std::size_t i = 0; i < count; ++i) {
        const float angle = (2.f * std::numbers::pi_v<float> * static_cast<float>(i))
                            / static_cast<float>(count);

        const sf::Vector2f remote {
            center.x + std::cos(angle) * remoteRadius,
            center.y + std::sin(angle) * remoteRadius
        };

        sf::Vector2f delta { remote.x - center.x, remote.y - center.y };
        const float length = std::sqrt(delta.x * delta.x + delta.y * delta.y);
        if (length <= 0.01f) continue;

        const float rotation = std::atan2(delta.y, delta.x) * 180.f / std::numbers::pi_v<float>;

        const float thickness = std::min(4.f, 1.f + static_cast<float>(peers[i].weight) * 0.25f);

        sf::RectangleShape edge({ length, thickness });
        edge.setPosition(center);
        edge.setRotation(sf::degrees(rotation));
        edge.setFillColor(statusColor(peers[i].status));
        window.draw(edge);

        const float nodeRadius = std::min(7.f, 3.f + static_cast<float>(peers[i].weight) * 0.22f);
        sf::CircleShape remoteNode(nodeRadius);
        remoteNode.setOrigin({ nodeRadius, nodeRadius });
        remoteNode.setPosition(remote);
        remoteNode.setFillColor({ 110, 160, 235 });
        window.draw(remoteNode);
    }

    sf::CircleShape localNode(7.f);
    localNode.setOrigin({ 7.f, 7.f });
    localNode.setPosition(center);
    localNode.setFillColor({ 220, 220, 220 });
    window.draw(localNode);

    // TODO M8: add moving particles along edges for traffic activity animation.
    // TODO M8: add mouse picking so clicking a remote node opens connection details.
}
