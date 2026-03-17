#pragma once
#include <SFML/Graphics.hpp>

class IRenderer
{
public:
    virtual ~IRenderer() = default;
    virtual void draw(sf::RenderWindow& window) = 0;
};
