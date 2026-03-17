#include <cstdlib>
#include <iostream>

#include "Utils/RootCheck.hpp"
#include "App/ApplicationController.hpp"

int main()
{
    // M2: Проверка root — первое, до любой инициализации
    if (!RootCheck::isRoot()) {
        std::cerr << "[NetPulse] Error: requires root privileges (uid=0)\n"
                  << "[NetPulse] Run: sudo ./NetPulseMonitor\n";
        return EXIT_FAILURE;
    }

    ApplicationController app;
    app.run();

    return EXIT_SUCCESS;
}
