#include "common/signal_handler.hpp"

#include <csignal>
#include <iostream>

// Define the global stop flag
std::atomic_bool _running{true};

void signalHandler(int signum)
{
    if (signum == SIGINT)
    {
        std::cout << "Received SIGINT, stopping all threads...\n";
        _running = false;
    }
}

void setupSignalHandler()
{
    std::signal(SIGINT, signalHandler);
}
