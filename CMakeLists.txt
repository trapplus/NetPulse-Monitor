cmake_minimum_required(VERSION 3.20)
project(NetPulseMonitor VERSION 0.1.0 LANGUAGES CXX)

# ── Стандарт ─────────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Флаги ────────────────────────────────────────────────────────────────────
add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O2 -DNDEBUG>
)

# ── Исходники ─────────────────────────────────────────────────────────────────
set(SOURCES
    src/main.cpp

    src/App/ApplicationController.cpp

    src/Data/SystemInfoProvider.cpp
    src/Data/PacketSnifferProvider.cpp
    src/Data/ExternalAPIProvider.cpp
    src/Data/NetworkDeviceProvider.cpp
    src/Data/ConnectionProvider.cpp

    src/Render/DashboardRenderer.cpp
    src/Render/ConnectionVisualizer.cpp
    src/Render/UI/Panel.cpp
    src/Render/UI/TextBlock.cpp
    src/Render/UI/Button.cpp

    src/Utils/Logger.cpp
    src/Utils/NetworkUtils.cpp
    src/Utils/RootCheck.cpp
)

add_executable(NetPulseMonitor ${SOURCES})

# ── Include dirs ──────────────────────────────────────────────────────────────
# Все хедеры через #include "App/Foo.hpp", #include "Utils/Bar.hpp" и т.д.
target_include_directories(NetPulseMonitor PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# ── SFML 2.6 ─────────────────────────────────────────────────────────────────
find_package(SFML 2.6 COMPONENTS graphics window system REQUIRED)

# ── libcurl (External IP API) ─────────────────────────────────────────────────
find_package(CURL REQUIRED)

# ── libpcap (Packet Sniffer) ──────────────────────────────────────────────────
# Нет официального CMake Find-модуля — ищем вручную
find_library(PCAP_LIBRARY
    NAMES pcap
    REQUIRED
)
find_path(PCAP_INCLUDE_DIR
    NAMES pcap.h
    REQUIRED
)

if(NOT PCAP_LIBRARY)
    message(FATAL_ERROR "libpcap not found. Install: sudo apt install libpcap-dev")
endif()

# ── Threads (std::thread, std::mutex, std::condition_variable) ───────────────
find_package(Threads REQUIRED)

# ── Линковка ──────────────────────────────────────────────────────────────────
target_link_libraries(NetPulseMonitor PRIVATE
    # SFML
    sfml-graphics
    sfml-window
    sfml-system

    # HTTP запросы к IP API
    CURL::libcurl

    # Перехват пакетов
    ${PCAP_LIBRARY}

    # Потоки (Data-thread, API-thread)
    Threads::Threads
)

target_include_directories(NetPulseMonitor PRIVATE
    ${PCAP_INCLUDE_DIR}
)

# ── Диагностика при конфигурации ──────────────────────────────────────────────
message(STATUS "")
message(STATUS "╔══════════════════════════════════════╗")
message(STATUS "║     NetPulse Monitor ${PROJECT_VERSION}           ║")
message(STATUS "╠══════════════════════════════════════╣")
message(STATUS "║  Build   : ${CMAKE_BUILD_TYPE}")
message(STATUS "║  C++     : ${CMAKE_CXX_STANDARD}")
message(STATUS "║  SFML    : ${SFML_VERSION}")
message(STATUS "║  CURL    : ${CURL_VERSION_STRING}")
message(STATUS "║  pcap    : ${PCAP_LIBRARY}")
message(STATUS "╚══════════════════════════════════════╝")
message(STATUS "")