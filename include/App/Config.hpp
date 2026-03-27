#pragma once
#include <SFML/Graphics/Color.hpp>
#include <array>
#include <chrono>

namespace Config {
inline constexpr unsigned int WINDOW_WIDTH  = 1180;
inline constexpr unsigned int WINDOW_HEIGHT = 860;
inline constexpr unsigned int MIN_WINDOW_WIDTH  = WINDOW_WIDTH;
inline constexpr unsigned int MIN_WINDOW_HEIGHT = WINDOW_HEIGHT;
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
inline constexpr float IFACE_BUTTON_PADDING_X  = 8.f;
inline constexpr float IFACE_BUTTON_PADDING_Y  = 5.f;
inline constexpr float IFACE_BUTTON_SPACING    = 6.f;
inline constexpr float MODE_BUTTON_PADDING_X   = 10.f;
inline constexpr float MODE_BUTTON_PADDING_Y   = 6.f;
inline constexpr float MODE_BUTTON_SPACING     = 6.f;
inline constexpr float SELECTOR_STRIP_PADDING_X = 8.f;
inline constexpr float SELECTOR_STRIP_PADDING_Y = 5.f;
inline constexpr float REQUEST_LOG_TIME_OFFSET   = 58.f;
inline constexpr float REQUEST_LOG_METHOD_OFFSET = 62.f;
inline constexpr float REQUEST_LOG_HOST_OFFSET   = 190.f;

inline constexpr float PANEL_PADDING = 10.f;
inline constexpr float PANEL_INNER_PADDING = 18.f;
inline constexpr float PANEL_CONTENT_TOP = 64.f;
inline constexpr float PANEL_TITLE_OFFSET_X = 14.f;
inline constexpr float PANEL_TITLE_OFFSET_Y = 12.f;
inline constexpr float PANEL_VISUALIZER_TOP = 88.f;
inline constexpr float PANEL_VISUALIZER_BOTTOM = 56.f;
inline constexpr float PANEL_VISUALIZER_SIDE_PADDING = 10.f;
inline constexpr float PANEL_OUTLINE_THICKNESS = 1.f;
inline constexpr float PANEL_CORNER_RADIUS = 12.f;
inline constexpr float BUTTON_CORNER_RADIUS = 7.f;
inline constexpr float PANEL_LINE_HEIGHT = 17.f;
inline constexpr float PANEL_TABLE_HEADER_HEIGHT = 15.f;
inline constexpr float EXTERNAL_API_CONTENT_TOP = 64.f;
inline constexpr float EXTERNAL_API_LINE_GAP = 18.f;
inline constexpr float CONNECTION_STATS_TOP = 32.f;
inline constexpr float CONNECTION_STATS_OFFSET_X = 12.f;
inline constexpr float SYSTEM_INFO_FIXED_HEIGHT = 214.f;
inline constexpr float NETWORK_DEVICES_FIXED_HEIGHT = 246.f;
inline constexpr float EXTERNAL_API_FIXED_HEIGHT = 156.f;
inline constexpr float NETWORK_SUMMARY_TOP = 68.f;
inline constexpr float NETWORK_SUMMARY_GAP = 6.f;
inline constexpr float NETWORK_TABLE_TOP = 112.f;
inline constexpr float NETWORK_FOOTER_HEIGHT = 22.f;
inline constexpr float CONTENT_SURFACE_INSET_X = 14.f;
inline constexpr float CONTENT_SURFACE_TOP_OFFSET = -6.f;
inline constexpr float CONTENT_SURFACE_BOTTOM_INSET = 16.f;
inline constexpr float CONTENT_SURFACE_PADDING_X = 14.f;
inline constexpr float CONTENT_SURFACE_PADDING_Y = 10.f;
inline constexpr float HEADER_KICKER_OFFSET_Y = 4.f;
inline constexpr float HEADER_TITLE_OFFSET_Y = 14.f;
inline constexpr float HEADER_DIVIDER_OFFSET_Y = 40.f;
inline constexpr float HEADER_DIVIDER_WIDTH = 44.f;
inline constexpr float STAT_CHIP_HEIGHT = 22.f;
inline constexpr float STAT_CHIP_GAP = 8.f;

inline constexpr unsigned int TITLE_FONT_SIZE = 17;
inline constexpr unsigned int BODY_FONT_SIZE = 11;
inline constexpr unsigned int CAPTION_FONT_SIZE = 10;
inline constexpr unsigned int KICKER_FONT_SIZE = 9;

inline constexpr float SYSTEM_INFO_VERSION_OFFSET = 110.f;
inline constexpr float SYSTEM_INFO_STATUS_OFFSET = 110.f;
inline constexpr float NETWORK_MAC_OFFSET = 135.f;
inline constexpr float NETWORK_IFACE_OFFSET = 290.f;
inline constexpr float NETWORK_STATUS_OFFSET = 95.f;
inline constexpr float CONNECTION_STAT_X = 88.f;

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
inline constexpr float CONNECTION_GROUP_LABEL_OFFSET_Y = 18.f;
inline constexpr float CONNECTION_GROUP_LABEL_OFFSET_X = 10.f;

inline constexpr sf::Color PANEL_FILL_COLOR { 30, 30, 34 };
inline constexpr sf::Color PANEL_BORDER_COLOR { 62, 62, 68 };
inline constexpr sf::Color PANEL_WAIT_COLOR { 95, 95, 102 };
inline constexpr sf::Color PANEL_SHADOW_COLOR { 0, 0, 0, 115 };
inline constexpr sf::Color BUTTON_IDLE_FILL_COLOR { 42, 42, 48 };
inline constexpr sf::Color SELECTOR_STRIP_FILL_COLOR { 36, 36, 40, 210 };
inline constexpr sf::Color ROW_CARD_FILL_COLOR { 38, 38, 44, 215 };
inline constexpr sf::Color ROW_CARD_OUTLINE_COLOR { 56, 56, 62, 215 };
inline constexpr sf::Color ROW_HOVER_FILL_COLOR { 48, 48, 55, 220 };
inline constexpr sf::Color CONTENT_SURFACE_FILL_COLOR { 35, 35, 40, 230 };
inline constexpr sf::Color CONTENT_SURFACE_OUTLINE_COLOR { 52, 52, 58, 220 };
inline constexpr sf::Color HEADER_ICON_FILL_COLOR { 245, 245, 245 };
inline constexpr sf::Color HEADER_DIVIDER_COLOR { 255, 183, 77 };
inline constexpr sf::Color TEXT_PRIMARY_COLOR { 235, 235, 236 };
inline constexpr sf::Color TEXT_SECONDARY_COLOR { 186, 186, 190 };
inline constexpr sf::Color TEXT_DIM_COLOR { 134, 134, 140 };
inline constexpr sf::Color STATUS_ACTIVE_COLOR { 120, 200, 120 };
inline constexpr sf::Color STATUS_INACTIVE_COLOR { 220, 100, 80 };
inline constexpr sf::Color STATUS_NOT_INSTALLED_COLOR { 134, 134, 140 };
inline constexpr sf::Color STATUS_LISTEN_COLOR { 255, 183, 77 };
inline constexpr sf::Color INTERFACE_ACCENT_COLOR { 205, 205, 208 };
inline constexpr sf::Color EXTERNAL_IP_COLOR { 235, 235, 236 };
inline constexpr sf::Color EXTERNAL_PROVIDER_COLOR { 186, 186, 190 };
inline constexpr sf::Color EXTERNAL_LOCATION_COLOR { 205, 205, 208 };
inline constexpr sf::Color CONNECTION_TCP_COLOR { 110, 210, 120 };
inline constexpr sf::Color CONNECTION_UDP_COLOR { 110, 170, 235 };
inline constexpr sf::Color CONNECTION_TOTAL_COLOR { 225, 190, 105 };
inline constexpr sf::Color REQUEST_LOG_POST_COLOR { 255, 183, 77 };
inline constexpr sf::Color REQUEST_LOG_DELETE_COLOR { 186, 186, 190 };
inline constexpr sf::Color REQUEST_LOG_PATCH_COLOR { 210, 140,  60 };
inline constexpr sf::Color REQUEST_LOG_OPTIONS_COLOR { 170, 170, 172 };
inline constexpr sf::Color CONNECTION_LOCAL_NODE_COLOR { 220, 220, 220 };
inline constexpr sf::Color TOOLTIP_FILL_COLOR { 20, 22, 28, 230 };
inline constexpr sf::Color TOOLTIP_OUTLINE_COLOR { 80, 90, 110 };
inline constexpr std::array<sf::Color, 5> PANEL_TITLE_COLORS = {{
    { 245, 245, 245 },
    { 245, 245, 245 },
    { 245, 245, 245 },
    { 245, 245, 245 },
    { 245, 245, 245 }
}};
inline constexpr std::array<sf::Color, 5> CONNECTION_NODE_COLORS = {{
    { 90, 180, 110 },
    { 90, 145, 220 },
    { 220, 165, 90 },
    { 165, 110, 210 },
    { 95, 195, 195 }
}};

inline constexpr std::array<const char*, 5> FONT_PATHS = {
    "/usr/share/fonts/noto/NotoSans-Medium.ttf",
    "/usr/share/fonts/noto/NotoSans-Regular.ttf",
    "/usr/share/fonts/noto/NotoSansMono-Medium.ttf",
    "/usr/share/fonts/noto/NotoSansMono-Regular.ttf",
    "/usr/share/fonts/TTF/DejaVuSansMono.ttf"
};
}
