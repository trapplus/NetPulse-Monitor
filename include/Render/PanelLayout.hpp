#pragma once
#include "App/Config.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <string_view>

namespace Panel {

enum class PanelId : std::size_t {
    SystemInfo = 0,
    RequestLog,
    ExternalIP,
    NetworkDevices,
    ConnectionVisualizer,
    Count
};

struct PanelLayout {
    std::string_view title;
    float            x;
    float            y;
    float            w;
    float            h;
};

using PanelLayoutArray = std::array<PanelLayout, static_cast<std::size_t>(PanelId::Count)>;
using LabelArray = std::array<std::string_view, 4>;

inline constexpr LabelArray kVisualizerModeLabels { "ALL", "TCP", "UDP", "STATE" };

inline std::string_view panelKicker(PanelId id)
{
    // Short labels keep the header calm while still hinting what each block is about.
    switch (id) {
        case PanelId::SystemInfo: return "RUNTIME";
        case PanelId::RequestLog: return "TRAFFIC";
        case PanelId::ExternalIP: return "UPLINK";
        case PanelId::NetworkDevices: return "LAN";
        case PanelId::ConnectionVisualizer: return "GRAPH";
        case PanelId::Count: break;
    }
    return "";
}

inline PanelLayoutArray makePanels(float width, float height)
{
    // Clamp to minimum window size so panel math stays sane even during aggressive resize drags.
    width = std::max(width, static_cast<float>(Config::MIN_WINDOW_WIDTH));
    height = std::max(height, static_cast<float>(Config::MIN_WINDOW_HEIGHT));

    const float pad = Config::PANEL_PADDING;
    const float colW = (width - pad * 3.f) * 0.5f;
    const float contentH = height - pad * 2.f;
    const float leftTopH = Config::SYSTEM_INFO_FIXED_HEIGHT;
    const float leftBottomH = contentH - pad - leftTopH;
    const float rightTopH = Config::NETWORK_DEVICES_FIXED_HEIGHT;
    const float rightMidH = Config::EXTERNAL_API_FIXED_HEIGHT;
    const float rightBottomH = contentH - pad * 2.f - rightTopH - rightMidH;
    const float xL = pad;
    const float xR = xL + colW + pad;

    return {{
        { "System Info", xL, pad, colW, leftTopH },
        { "Request Log", xL, pad + leftTopH + pad, colW, leftBottomH },
        { "External IP", xR, pad + rightTopH + pad, colW, rightMidH },
        { "Network Devices", xR, pad, colW, rightTopH },
        { "Connection Visualizer", xR, pad + rightTopH + pad + rightMidH + pad, colW, rightBottomH },
    }};
}

inline const PanelLayout& panelFor(const PanelLayoutArray& panels, PanelId id)
{
    return panels[static_cast<std::size_t>(id)];
}

} // namespace Panel
