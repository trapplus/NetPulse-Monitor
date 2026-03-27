cmake_minimum_required(VERSION 3.20)
project(NetPulseMonitor VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wpedantic -Wshadow)

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
)

add_executable(NetPulseMonitor ${SOURCES})

target_compile_options(NetPulseMonitor PRIVATE
    $<$<CONFIG:Debug>:-g>
    $<$<CONFIG:Debug>:-O0>
    $<$<CONFIG:Release>:-O2>
    $<$<CONFIG:Release>:-DNDEBUG>
)

target_include_directories(NetPulseMonitor PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

find_package(SFML 3 COMPONENTS Graphics Window System REQUIRED)
find_package(CURL REQUIRED)

find_library(PCAP_LIBRARY NAMES pcap REQUIRED)
find_path(PCAP_INCLUDE_DIR NAMES pcap.h REQUIRED)
if(NOT PCAP_LIBRARY)
    message(FATAL_ERROR "libpcap not found. Install: sudo pacman -S libpcap")
endif()

find_package(Threads REQUIRED)

target_link_libraries(NetPulseMonitor PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
    CURL::libcurl
    ${PCAP_LIBRARY}
    Threads::Threads
)

target_include_directories(NetPulseMonitor PRIVATE ${PCAP_INCLUDE_DIR})

message(STATUS "-- NetPulse Monitor ${PROJECT_VERSION} --")
message(STATUS "Build: ${CMAKE_BUILD_TYPE} | C++${CMAKE_CXX_STANDARD}")
