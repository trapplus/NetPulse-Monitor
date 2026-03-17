#pragma once
#include <SFML/Graphics.hpp>

class ApplicationController {
public:
    ApplicationController();
    ~ApplicationController();
    void run();

private:
    void processEvents();
    void update();
    void render();
    void renderPlaceholders();

    sf::RenderWindow m_window;
    sf::Font         m_font;
    bool             m_fontLoaded = false;
};
