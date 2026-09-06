#pragma once

#include "listening_port.hpp"

#include <cstddef>
#include <vector>

namespace devdock {
    // The outcome of one listener scan. Inspection is best effort: a process that
    // exits during the scan is ignored, while a process this user is not allowed
    // to inspect is counted, so that callers can qualify an absent listener
    // instead of reporting it as a certainty. A scan is never an atomic snapshot
    // of the operating system.
    struct PortScan {
        std::vector<ListeningPort> listeners;
        std::size_t scanned_processes = 0;
        std::size_t uninspectable_processes = 0;
    };
}
