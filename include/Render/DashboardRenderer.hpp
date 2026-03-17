#pragma once
#include "Render/IRenderer.hpp"

class DashboardRenderer : public IRenderer
{
public:
    DashboardRenderer();
    ~DashboardRenderer() override;
    void draw(sf::RenderWindow& window) override;
};
