#pragma once
#include <SFML/Graphics/RenderWindow.hpp>

namespace UI {

class Panel
{
public:
    Panel();
    void draw(sf::RenderWindow& window);
};

} // namespace UI
