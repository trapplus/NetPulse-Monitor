#pragma once

// all magic numbers live here - change once, applies everywhere
namespace Config {
    constexpr unsigned int WINDOW_WIDTH  = 1280;
    constexpr unsigned int WINDOW_HEIGHT = 720;
    constexpr unsigned int TARGET_FPS    = 60;
    constexpr const char*  WINDOW_TITLE  = "NetPulse Monitor";

    // background color - dark but not pure black
    constexpr unsigned int BG_R = 18;
    constexpr unsigned int BG_G = 18;
    constexpr unsigned int BG_B = 18;

    // how often providers refresh their data (seconds)
    constexpr float API_REFRESH_SEC  = 300.f;  // external IP - 5 min is fine
    constexpr float DATA_REFRESH_SEC = 2.f;

    // max entries kept in the request log sliding window
    constexpr int REQUEST_LOG_LIMIT = 100;
}
