#pragma once

#include "domain/error.hpp"
#include "domain/process_identity.hpp"

#include <chrono>
#include <cstdint>

namespace devdock {
    enum class TerminationMode : std::uint8_t {
        graceful,
        force,
    };

    class ProcessController {
    public:
        virtual ~ProcessController() = default;

    protected:
        ProcessController() = default;
        ProcessController(const ProcessController&) = default;
        ProcessController(ProcessController&&) = default;
        ProcessController& operator=(const ProcessController&) = default;
        ProcessController& operator=(ProcessController&&) = default;

    public:
        [[nodiscard]] virtual Result<bool> stop(
            const ProcessIdentity& identity,
            TerminationMode mode,
            std::chrono::milliseconds timeout
        ) const = 0;
    };
}