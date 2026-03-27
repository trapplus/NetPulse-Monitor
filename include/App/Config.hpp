#pragma once
#include <SFML/Graphics/Color.hpp>
#include <array>
#include <chrono>

namespace Config {
inline constexpr unsigned int WINDOW_WIDTH  = 1280;
inline constexpr unsigned int WINDOW_HEIGHT = 720;
inline constexpr unsigned int TARGET_FPS    = 60;
inline constexpr const char*  WINDOW_TITLE  = "NetPulse Monitor";

inline constexpr unsigned int BG_R = 18;
inline constexpr unsigned int BG_G = 18;
inline constexpr unsigned int BG_B = 18;

inline constexpr std::chrono::seconds SYSTEM_INFO_REFRESH_INTERVAL { 30 };
inline constexpr std::chrono::seconds API_REFRESH_INTERVAL { 300 };
inline constexpr std::chrono::seconds DATA_REFRESH_INTERVAL { 2 };
inline constexpr std::chrono::milliseconds WORKER_SLEEP_INTERVAL { 250 };
inline constexpr std::chrono::milliseconds API_CONNECT_TIMEOUT { 3000 };
inline constexpr std::chrono::milliseconds API_REQUEST_TIMEOUT { 5000 };

inline constexpr std::size_t REQUEST_LOG_LIMIT = 100;

// packet sniffer UI
inline constexpr float IFACE_BUTTON_PADDING_X  = 4.f;
inline constexpr float IFACE_BUTTON_PADDING_Y  = 2.f;
inline constexpr float IFACE_BUTTON_SPACING    = 4.f;
inline constexpr float REQUEST_LOG_TIME_OFFSET   = 58.f;
inline constexpr float REQUEST_LOG_METHOD_OFFSET = 62.f;
inline constexpr float REQUEST_LOG_HOST_OFFSET   = 190.f;

inline constexpr float PANEL_PADDING = 12.f;
inline constexpr float PANEL_INNER_PADDING = 14.f;
inline constexpr float PANEL_CONTENT_TOP = 30.f;
inline constexpr float PANEL_TITLE_OFFSET_X = 10.f;
inline constexpr float PANEL_TITLE_OFFSET_Y = 8.f;
inline constexpr float PANEL_VISUALIZER_TOP = 48.f;
inline constexpr float PANEL_VISUALIZER_BOTTOM = 56.f;
inline constexpr float PANEL_VISUALIZER_SIDE_PADDING = 8.f;
inline constexpr float PANEL_OUTLINE_THICKNESS = 1.f;
inline constexpr float PANEL_LINE_HEIGHT = 16.f;
inline constexpr float EXTERNAL_API_CONTENT_TOP = 32.f;
inline constexpr float CONNECTION_STATS_TOP = 28.f;
inline constexpr float CONNECTION_STATS_OFFSET_X = 12.f;

inline constexpr unsigned int TITLE_FONT_SIZE = 13;
inline constexpr unsigned int BODY_FONT_SIZE = 11;

inline constexpr float SYSTEM_INFO_VERSION_OFFSET = 110.f;
inline constexpr float SYSTEM_INFO_STATUS_OFFSET = 110.f;
inline constexpr float NETWORK_MAC_OFFSET = 135.f;
inline constexpr float NETWORK_IFACE_OFFSET = 290.f;
inline constexpr float NETWORK_STATUS_OFFSET = 95.f;
inline constexpr float CONNECTION_STAT_X = 88.f;

inline constexpr std::size_t MAX_CONNECTION_VISUALIZER_PEERS = 22;
inline constexpr float CONNECTION_EDGE_MAX_THICKNESS = 4.f;
inline constexpr float CONNECTION_EDGE_BASE_THICKNESS = 1.f;
inline constexpr float CONNECTION_EDGE_WEIGHT_SCALE = 0.25f;
inline constexpr float CONNECTION_NODE_BASE_RADIUS = 3.f;
inline constexpr float CONNECTION_NODE_MAX_RADIUS = 7.f;
inline constexpr float CONNECTION_NODE_WEIGHT_SCALE = 0.22f;
inline constexpr float CONNECTION_HIT_RADIUS_PADDING = 4.f;
inline constexpr float CONNECTION_LOCAL_NODE_RADIUS = 7.f;
inline constexpr float CONNECTION_CENTER_RATIO = 0.5f;
inline constexpr float CONNECTION_MIN_REMOTE_RING_RADIUS = 30.f;
inline constexpr float CONNECTION_REMOTE_RING_RATIO = 0.35f;
inline constexpr float CONNECTION_MIN_EDGE_LENGTH = 0.01f;
inline constexpr float CONNECTION_DEGREES_PER_RADIAN = 180.f;
inline constexpr float CONNECTION_TOOLTIP_OFFSET_X = 12.f;
inline constexpr float CONNECTION_TOOLTIP_OFFSET_Y = -10.f;
inline constexpr float CONNECTION_TOOLTIP_PADDING_X = 10.f;
inline constexpr float CONNECTION_TOOLTIP_PADDING_Y = 8.f;

inline constexpr sf::Color PANEL_FILL_COLOR { 26, 26, 30 };
inline constexpr sf::Color PANEL_BORDER_COLOR { 55, 55, 60 };
inline constexpr sf::Color PANEL_WAIT_COLOR { 60, 60, 65 };
inline constexpr sf::Color TEXT_PRIMARY_COLOR { 200, 200, 200 };
inline constexpr sf::Color TEXT_SECONDARY_COLOR { 160, 160, 160 };
inline constexpr sf::Color TEXT_DIM_COLOR { 120, 120, 125 };
inline constexpr sf::Color STATUS_ACTIVE_COLOR { 120, 200, 120 };
inline constexpr sf::Color STATUS_INACTIVE_COLOR { 200, 160, 60 };
inline constexpr sf::Color STATUS_NOT_INSTALLED_COLOR { 120, 120, 120 };
inline constexpr sf::Color STATUS_LISTEN_COLOR { 200, 180, 80 };
inline constexpr sf::Color INTERFACE_ACCENT_COLOR { 120, 160, 220 };
inline constexpr sf::Color EXTERNAL_IP_COLOR { 205, 205, 205 };
inline constexpr sf::Color EXTERNAL_PROVIDER_COLOR { 170, 170, 170 };
inline constexpr sf::Color EXTERNAL_LOCATION_COLOR { 130, 170, 230 };
inline constexpr sf::Color CONNECTION_TCP_COLOR { 110, 210, 120 };
inline constexpr sf::Color CONNECTION_UDP_COLOR { 110, 170, 235 };
inline constexpr sf::Color CONNECTION_TOTAL_COLOR { 225, 190, 105 };
inline constexpr sf::Color REQUEST_LOG_POST_COLOR { 220, 100,  80 };
inline constexpr sf::Color REQUEST_LOG_DELETE_COLOR { 100, 140, 220 };
inline constexpr sf::Color REQUEST_LOG_PATCH_COLOR { 210, 140,  60 };
inline constexpr sf::Color REQUEST_LOG_OPTIONS_COLOR { 160, 100, 210 };
inline constexpr sf::Color CONNECTION_LOCAL_NODE_COLOR { 220, 220, 220 };
inline constexpr sf::Color TOOLTIP_FILL_COLOR { 20, 22, 28, 230 };
inline constexpr sf::Color TOOLTIP_OUTLINE_COLOR { 80, 90, 110 };
inline constexpr std::array<sf::Color, 5> PANEL_TITLE_COLORS = {{
    { 100, 180, 100 },
    { 130, 165, 230 },
    { 215, 175, 105 },
    { 170, 135, 215 },
    { 105, 195, 195 }
}};
inline constexpr std::array<sf::Color, 5> CONNECTION_NODE_COLORS = {{
    { 90, 180, 110 },
    { 90, 145, 220 },
    { 220, 165, 90 },
    { 165, 110, 210 },
    { 95, 195, 195 }
}};

inline constexpr std::array<const char*, 3> FONT_PATHS = {
    "/usr/share/fonts/TTF/DejaVuSansMono.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
    "/usr/share/fonts/dejavu/DejaVuSansMono.ttf"
};
}
