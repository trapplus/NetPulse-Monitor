#pragma once
#include "Data/ConnectionProvider.hpp"
#include "Render/IRenderer.hpp"
#include <SFML/Graphics/Font.hpp>
#include <string>
#include <vector>

class ConnectionVisualizer : public IRenderer
{
public:
    ConnectionVisualizer();
    ~ConnectionVisualizer() override;

    void setViewport(const sf::FloatRect& viewport);
    void setConnections(const std::vector<ConnectionInfo>& connections);
    void setFont(const sf::Font* font);

    void draw(sf::RenderWindow& window) override;

private:
    struct PeerSummary {
        std::string            ip;
        ConnectionInfo::Status status { ConnectionInfo::Status::OTHER };
        int                    weight { 0 };
        int                    tcpCount { 0 };
        int                    udpCount { 0 };
    };

    static sf::Color statusColor(ConnectionInfo::Status status);
    static std::vector<PeerSummary> buildPeerSummaries(const std::vector<ConnectionInfo>& connections);
    static const char* statusLabel(ConnectionInfo::Status status);
    static sf::Color headerColorForIndex(std::size_t index);

    sf::FloatRect                m_viewport;
    std::vector<ConnectionInfo>  m_connections;
    const sf::Font*              m_font { nullptr };
};
