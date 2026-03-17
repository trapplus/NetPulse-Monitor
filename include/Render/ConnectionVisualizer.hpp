#pragma once
#include "Render/IRenderer.hpp"

class ConnectionVisualizer : public IRenderer
{
public:
    ConnectionVisualizer();
    ~ConnectionVisualizer() override;
    void draw(sf::RenderWindow& window) override;
};
