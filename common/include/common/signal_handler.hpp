#pragma once

#include <atomic>

// Declare the global stop flag (extern means it's defined elsewhere)
extern std::atomic_bool _running;

void setupSignalHandler();
