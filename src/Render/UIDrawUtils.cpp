#include "Render/UIDrawUtils.hpp"
#include "App/Config.hpp"
#include <algorithm>

namespace Draw {

void drawRoundedRect(sf::RenderWindow& window,
                     const sf::FloatRect& rect,
                     float radius,
                     const sf::Color& fillColor)
{
    // We build rounded rectangles from primitive shapes so we don't depend on extra geometry utilities.
    const float clampedRadius = std::min(radius, std::min(rect.size.x, rect.size.y) * 0.5f);
    if (clampedRadius <= 0.f) {
        sf::RectangleShape box(rect.size);
        box.setPosition(rect.position);
        box.setFillColor(fillColor);
        window.draw(box);
        return;
    }

    sf::RectangleShape horizontal({ rect.size.x - clampedRadius * 2.f, rect.size.y });
    horizontal.setPosition({ rect.position.x + clampedRadius, rect.position.y });
    horizontal.setFillColor(fillColor);
    window.draw(horizontal);

    sf::RectangleShape vertical({ rect.size.x, rect.size.y - clampedRadius * 2.f });
    vertical.setPosition({ rect.position.x, rect.position.y + clampedRadius });
    vertical.setFillColor(fillColor);
    window.draw(vertical);

    for (const sf::Vector2f center : {
             sf::Vector2f { rect.position.x + clampedRadius, rect.position.y + clampedRadius },
             sf::Vector2f { rect.position.x + rect.size.x - clampedRadius, rect.position.y + clampedRadius },
             sf::Vector2f { rect.position.x + clampedRadius, rect.position.y + rect.size.y - clampedRadius },
             sf::Vector2f { rect.position.x + rect.size.x - clampedRadius, rect.position.y + rect.size.y - clampedRadius },
         }) {
        sf::CircleShape corner(clampedRadius, 24);
        corner.setOrigin({ clampedRadius, clampedRadius });
        corner.setPosition(center);
        corner.setFillColor(fillColor);
        window.draw(corner);
    }
}

void drawRoundedFrame(sf::RenderWindow& window, const sf::FloatRect& rect)
{
    // Draw shadow first to fake depth; the thin border layer keeps dark panels from blending together.
    const sf::FloatRect shadowRect {
        { rect.position.x, rect.position.y + 2.f },
        rect.size
    };
    drawRoundedRect(window, shadowRect, Config::PANEL_CORNER_RADIUS, Config::PANEL_SHADOW_COLOR);
    drawRoundedRect(window, rect, Config::PANEL_CORNER_RADIUS, Config::PANEL_BORDER_COLOR);

    const float inset = Config::PANEL_OUTLINE_THICKNESS;
    drawRoundedRect(window,
                    sf::FloatRect {
                        { rect.position.x + inset, rect.position.y + inset },
                        { rect.size.x - inset * 2.f, rect.size.y - inset * 2.f }
                    },
                    std::max(0.f, Config::PANEL_CORNER_RADIUS - inset),
                    Config::PANEL_FILL_COLOR);
}

void drawPillButton(sf::RenderWindow& window,
                    const sf::FloatRect& bounds,
                    bool selected)
{
    drawRoundedRect(window,
                    bounds,
                    Config::BUTTON_CORNER_RADIUS,
                    selected ? Config::STATUS_ACTIVE_COLOR : Config::BUTTON_IDLE_FILL_COLOR);
}

void drawContentSurface(sf::RenderWindow& window, const sf::FloatRect& bounds)
{
    // Two-pass fill gives a subtle outline without requiring per-pixel shaders.
    drawRoundedRect(window,
                    bounds,
                    Config::BUTTON_CORNER_RADIUS + 2.f,
                    Config::CONTENT_SURFACE_OUTLINE_COLOR);
    drawRoundedRect(window,
                    sf::FloatRect {
                        { bounds.position.x + 1.f, bounds.position.y + 1.f },
                        { bounds.size.x - 2.f, bounds.size.y - 2.f }
                    },
                    std::max(0.f, Config::BUTTON_CORNER_RADIUS + 1.f),
                    Config::CONTENT_SURFACE_FILL_COLOR);
}

void drawHoverRow(sf::RenderWindow& window, const sf::FloatRect& bounds)
{
    drawRoundedRect(window,
                    bounds,
                    Config::BUTTON_CORNER_RADIUS,
                    Config::ROW_HOVER_FILL_COLOR);
}

sf::FloatRect makeContentSurfaceRect(const Panel::PanelLayout& panel, float contentTop)
{
    // Centralizing this math keeps every block renderer aligned when spacing constants change.
    return sf::FloatRect {
        { panel.x + Config::CONTENT_SURFACE_INSET_X, panel.y + contentTop + Config::CONTENT_SURFACE_TOP_OFFSET },
        {
            panel.w - Config::CONTENT_SURFACE_INSET_X * 2.f,
            panel.h - contentTop - Config::CONTENT_SURFACE_BOTTOM_INSET
        }
    };
}

void drawPanelHeader(sf::RenderWindow& window,
                     const sf::Font& titleFont,
                     const Panel::PanelLayout& panel,
                     Panel::PanelId id)
{
    // Kicker + title mirrors dashboard cards from observability tools and helps scanning speed.
    const float textX = panel.x + Config::PANEL_TITLE_OFFSET_X;

    sf::Text kicker(titleFont, std::string(Panel::panelKicker(id)), Config::KICKER_FONT_SIZE);
    kicker.setFillColor(Config::TEXT_DIM_COLOR);
    kicker.setLetterSpacing(1.3f);
    kicker.setPosition({ textX, panel.y + Config::PANEL_TITLE_OFFSET_Y + Config::HEADER_KICKER_OFFSET_Y });
    window.draw(kicker);

    sf::Text title(titleFont, std::string(panel.title), Config::TITLE_FONT_SIZE);
    title.setFillColor(Config::HEADER_ICON_FILL_COLOR);
    title.setPosition({ textX, panel.y + Config::PANEL_TITLE_OFFSET_Y + Config::HEADER_TITLE_OFFSET_Y });
    window.draw(title);
}

} // namespace Draw
