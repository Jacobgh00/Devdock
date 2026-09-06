#pragma once

#include "process_identity.hpp"
#include "protocol.hpp"

namespace devdock {
    struct ListeningPort {
        std::uint16_t port;
        Protocol protocol;
        std::string address;
        ProcessId pid;
    };
}