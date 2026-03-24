#pragma once
#include "Data/ConnectionProvider.hpp"
#include "Render/IRenderer.hpp"
#include <string>
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
    struct PeerSummary {
        std::string            ip;
        ConnectionInfo::Status status { ConnectionInfo::Status::OTHER };
        int                    weight { 0 };
    };

    static sf::Color statusColor(ConnectionInfo::Status status);
    static std::vector<PeerSummary> buildPeerSummaries(const std::vector<ConnectionInfo>& connections);

    sf::FloatRect                m_viewport;
    std::vector<ConnectionInfo>  m_connections;
};
