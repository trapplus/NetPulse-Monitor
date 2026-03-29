#pragma once
#include "Data/ConnectionInfo.hpp"
#include "Data/ExternalAPIProvider.hpp"
#include "Data/NetworkDeviceProvider.hpp"
#include "Data/RequestEntry.hpp"
#include "Data/SystemInfoProvider.hpp"
#include "Render/ConnectionVisualizer.hpp"
#include "Render/PanelLayout.hpp"
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

namespace BlockRender {

// Each block gets a focused entry point so ApplicationController stays a coordinator only.
void systemInfo(sf::RenderWindow& window,
                const sf::Font& font,
                const Panel::PanelLayout& panel,
                const std::vector<ToolInfo>& tools);

void requestLog(sf::RenderWindow& window,
                const sf::Font& font,
                const Panel::PanelLayout& panel,
                const std::vector<std::string>& interfaces,
                std::size_t selectedInterface,
                std::vector<sf::FloatRect>& ifaceButtonBoundsOut,
                const std::vector<RequestEntry>& entries);

void networkDevices(sf::RenderWindow& window,
                    const sf::Font& font,
                    const Panel::PanelLayout& panel,
                    const std::vector<NetworkDeviceInfo>& devices);

void externalAPI(sf::RenderWindow& window,
                 const sf::Font& font,
                 const Panel::PanelLayout& panel,
                 const ExternalAPIProvider::Snapshot& snapshot);

void connections(sf::RenderWindow& window,
                 const sf::Font* font,
                 const Panel::PanelLayout& panel,
                 const std::vector<ConnectionInfo>& connections,
                 std::size_t selectedMode,
                 std::vector<sf::FloatRect>& modeButtonBoundsOut,
                 ConnectionVisualizer& visualizer);

} // namespace BlockRender
