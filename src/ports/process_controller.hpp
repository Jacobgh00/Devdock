#pragma once

#include "domain/error.hpp"
#include "domain/process_identity.hpp"

#include <chrono>

namespace devdock {
    enum class TerminationMode {
        graceful,
        force,
    };

    class ProcessController {
    public:
        virtual ~ProcessController() = default;

        virtual Result<bool> stop(
            const ProcessIdentity& identity,
            TerminationMode mode,
            std::chrono::milliseconds timeout
        ) const = 0;
    };
}