#pragma once
#include "Render/PanelLayout.hpp"
#include <SFML/Graphics.hpp>

namespace Draw {

// Kept as free helpers so block renderers can share visuals without inheriting UI base classes.
void drawRoundedRect(sf::RenderWindow& window,
                     const sf::FloatRect& rect,
                     float radius,
                     const sf::Color& fillColor);
void drawRoundedFrame(sf::RenderWindow& window, const sf::FloatRect& rect);
void drawPillButton(sf::RenderWindow& window,
                    const sf::FloatRect& bounds,
                    bool selected);
void drawContentSurface(sf::RenderWindow& window, const sf::FloatRect& bounds);
void drawHoverRow(sf::RenderWindow& window, const sf::FloatRect& bounds);
sf::FloatRect makeContentSurfaceRect(const Panel::PanelLayout& panel, float contentTop);
void drawPanelHeader(sf::RenderWindow& window,
                     const sf::Font& titleFont,
                     const Panel::PanelLayout& panel,
                     Panel::PanelId id);

} // namespace Draw
