#pragma once

#include "domain/listening_port.hpp"
#include "domain/process.hpp"

#include <optional>

namespace devdock {
    struct PortDetails {
        ListeningPort listener;
        std::optional<Process> process;
    };
}