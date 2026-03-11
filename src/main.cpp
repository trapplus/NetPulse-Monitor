#include <cstdlib>
#include <iostream>
#include <unistd.h>

#include "Utils/RootCheck.hpp"

int main()
{
    // M2: Проверка root — первое, что происходит
    if (getuid() != 0) {
        std::cerr << "[NetPulse] Error: requires root privileges (uid=0)\n";
        return EXIT_FAILURE;
    }

    // TODO M2: инициализация ApplicationController и главный цикл

    return EXIT_SUCCESS;
}