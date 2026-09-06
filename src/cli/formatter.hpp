#pragma once

#include "domain/error.hpp"
#include "use_cases/kill_process.hpp"
#include "use_cases/port_details.hpp"

#include <ostream>
#include <string_view>
#include <vector>

namespace devdock {

    void print_usage(
        std::ostream& output
    );

    void print_argument_error(
        std::ostream& output,
        std::string_view message
    );

    void print_error(
        std::ostream& output,
        const Error& error
    );

    void print_ports(
        std::ostream& output,
        const std::vector<PortDetails>& ports
    );

    void print_port_details(
        std::ostream& output,
        const std::vector<PortDetails>& details
    );

    void print_kill_result(
        std::ostream& output,
        const KillResult& result
    );

    // Reports a process that outlived the signal it was sent. Exhaustive over the
    // termination mode: escalation is advised only after a graceful timeout.
    void print_stop_timeout(
        std::ostream& output,
        const KillResult& result
    );

} // namespace devdock