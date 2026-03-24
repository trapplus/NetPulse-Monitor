#pragma once
#include "Data/ConnectionProvider.hpp"
#include "Render/IRenderer.hpp"
#include <vector>

class ConnectionVisualizer : public IRenderer
{
public:
    ConnectionVisualizer();
    ~ConnectionVisualizer() override;

    void setViewport(const sf::FloatRect& viewport);
    void setConnections(const std::vector<ConnectionInfo>& connections);

    void draw(sf::RenderWindow& window) override;

private:
    static sf::Color statusColor(ConnectionInfo::Status status);

    sf::FloatRect                m_viewport;
    std::vector<ConnectionInfo>  m_connections;
};
