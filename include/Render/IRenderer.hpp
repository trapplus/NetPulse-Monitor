#pragma once
#include <SFML/Graphics/RenderWindow.hpp>

class IRenderer
{
public:
    virtual ~IRenderer() = default;
    virtual void draw(sf::RenderWindow& window) = 0;
};
