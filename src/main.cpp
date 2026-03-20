#include <cstdlib>   // EXIT_FAILURE - explicit, not relying on transitive includes
#include <iostream>

#include "Utils/RootCheck.hpp"
#include "App/ApplicationController.hpp"

int main()
{
    // root check must be first - pcap and raw sockets need it
    if (!RootCheck::isRoot()) {
        std::cerr << "[NetPulse] Error: requires root privileges (uid=0)\n"
                  << "[NetPulse] Run: sudo ./NetPulseMonitor\n";
        return EXIT_FAILURE;
    }

    ApplicationController app;
    app.run();

    return EXIT_SUCCESS;
}
