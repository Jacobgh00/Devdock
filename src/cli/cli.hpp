#pragma once

#include "ports/port_inspector.hpp"
#include "ports/process_controller.hpp"
#include "ports/process_inspector.hpp"

namespace devdock {

    int run_cli(
        int argc,
        char* argv[],
        const PortInspector& port_inspector,
        const ProcessInspector& process_inspector,
        const ProcessController& process_controller
    );

} // namespace devdock