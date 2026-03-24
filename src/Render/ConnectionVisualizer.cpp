#include "Render/ConnectionVisualizer.hpp"
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <cmath>
#include <numbers>

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

void ConnectionVisualizer::draw(sf::RenderWindow& window)
{
    if (m_viewport.size.x <= 0.f || m_viewport.size.y <= 0.f)
        return;

    const sf::Vector2f center {
        m_viewport.position.x + m_viewport.size.x * 0.5f,
        m_viewport.position.y + m_viewport.size.y * 0.5f
    };

    const float minDim = std::min(m_viewport.size.x, m_viewport.size.y);
    const float remoteRadius = std::max(30.f, minDim * 0.32f);

    const std::size_t count = m_connections.size();
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

        sf::RectangleShape edge({ length, 2.f });
        edge.setPosition(center);
        edge.setRotation(sf::degrees(rotation));
        edge.setFillColor(statusColor(m_connections[i].status));
        window.draw(edge);

        sf::CircleShape remoteNode(4.f);
        remoteNode.setOrigin({ 4.f, 4.f });
        remoteNode.setPosition(remote);
        remoteNode.setFillColor({ 90, 140, 220 });
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
