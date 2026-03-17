#pragma once

namespace Config {
    // Окно
    constexpr unsigned int WINDOW_WIDTH  = 1280;
    constexpr unsigned int WINDOW_HEIGHT = 720;
    constexpr unsigned int TARGET_FPS    = 60;
    constexpr const char*  WINDOW_TITLE  = "NetPulse Monitor";

    // Фон (тёмный)
    constexpr unsigned int BG_R = 18;
    constexpr unsigned int BG_G = 18;
    constexpr unsigned int BG_B = 18;

    // Таймеры
    constexpr float API_REFRESH_SEC  = 300.f;
    constexpr float DATA_REFRESH_SEC = 2.f;

    // Request Log
    constexpr int REQUEST_LOG_LIMIT = 100;
}
