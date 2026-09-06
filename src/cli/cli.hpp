#pragma once

#include "ports/port_inspector.hpp"
#include "ports/process_controller.hpp"
#include "ports/process_inspector.hpp"

#include <span>

namespace devdock {

    int run_cli(
        std::span<char* const> arguments,
        const PortInspector& port_inspector,
        const ProcessInspector& process_inspector,
        const ProcessController& process_controller
    );

} // namespace devdock