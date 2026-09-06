#pragma once

#include "domain/error.hpp"
#include "domain/process.hpp"
#include "ports/port_inspector.hpp"
#include "ports/process_controller.hpp"
#include "ports/process_inspector.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace devdock {

    struct KillResult {
        ProcessId pid;
        std::optional<std::uint16_t> port;
        std::string process_name;
        StopOutcome outcome;
        TerminationMode mode;
    };

    class KillProcess {
    public:
        KillProcess(
            const PortInspector& port_inspector,
            const ProcessInspector& process_inspector,
            const ProcessController& process_controller
        )
            : port_inspector_{port_inspector},
              process_inspector_{process_inspector},
              process_controller_{process_controller} {}

        [[nodiscard]] Result<KillResult> by_port(
            std::uint16_t port,
            TerminationMode mode
        ) const;

        [[nodiscard]] Result<KillResult> by_pid(
            ProcessId pid,
            TerminationMode mode
        ) const;

    private:
        [[nodiscard]] Result<void> confirm_owner(
            std::uint16_t port,
            const ProcessIdentity& identity
        ) const;

        [[nodiscard]] Result<KillResult> stop_process(
            const Process& process,
            std::optional<std::uint16_t> port,
            TerminationMode mode
        ) const;

        const PortInspector& port_inspector_;
        const ProcessInspector& process_inspector_;
        const ProcessController& process_controller_;
    };

}
