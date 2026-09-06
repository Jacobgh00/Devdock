#include "cli/formatter.hpp"

#include "cli/terminal_text.hpp"
#include "domain/protocol.hpp"

#include <iomanip>
#include <string>
#include <string_view>

namespace devdock {

    namespace {

        std::string_view protocol_name(
            Protocol protocol
        ) {
            return protocol == Protocol::tcp
                       ? "TCP"
                       : "TCP6";
        }

        std::string process_name(
            const PortDetails& details
        ) {
            if (!details.process) {
                return "<unavailable>";
            }

            return sanitize_terminal_text(
                details.process->name
            );
        }

        std::string working_directory(
            const PortDetails& details
        ) {
            if (
                !details.process || !details.process
                                         ->working_directory
            ) {
                return "<unavailable>";
            }

            return sanitize_terminal_text(
                details.process
                    ->working_directory
                    ->string()
            );
        }

        void print_single_port_details(
            std::ostream& output,
            const PortDetails& details
        ) {
            output
                << "Port       "
                << details.listener.port
                << '\n'

                << "Protocol   "
                << protocol_name(
                       details.listener.protocol
                   )
                << '\n'

                << "Address    "
                << sanitize_terminal_text(
                       details.listener.address
                   )
                << '\n'

                << "PID        "
                << details.listener.pid
                << '\n';

            if (!details.process) {
                output
                    << "Process    <unavailable>\n"
                    << "Command    <unavailable>\n"
                    << "CWD        <unavailable>\n";

                return;
            }

            output
                << "Process    "
                << sanitize_terminal_text(
                       details.process->name
                   )
                << '\n'

                << "Command    "
                << sanitize_terminal_text(
                       details.process->command
                   )
                << '\n'

                << "CWD        "
                << working_directory(
                       details
                   )
                << '\n';
        }

    } // namespace

    void print_usage(
        std::ostream& output
    ) {
        output
            << "DevDock - inspect and safely stop processes "
               "that own listening TCP ports\n\n"

            << "Usage:\n"
            << "  devdock ports\n"
            << "  devdock port <port>\n"
            << "  devdock kill <port> [--force]\n"
            << "  devdock kill --pid <pid> [--force]\n\n"

            << "Safety:\n"
            << "  Destructive commands refuse root and "
               "processes owned by other users.\n";
    }

    void print_argument_error(
        std::ostream& output,
        std::string_view message
    ) {
        output
            << "devdock: "
            << sanitize_terminal_text(
                   message
               )
            << '\n';
    }

    void print_error(
        std::ostream& output,
        const Error& error
    ) {
        output
            << "devdock: "
            << sanitize_terminal_text(
                   error.message
               )
            << '\n';
    }

    void print_ports(
        std::ostream& output,
        const std::vector<PortDetails>& ports
    ) {
        output
            << std::left
            << std::setw(8)
            << "PORT"

            << std::setw(8)
            << "PROTO"

            << std::setw(10)
            << "PID"

            << std::setw(24)
            << "PROCESS"

            << "ADDRESS\n";

        for (const auto& details : ports) {
            output
                << std::left

                << std::setw(8)
                << details.listener.port

                << std::setw(8)
                << protocol_name(
                       details.listener.protocol
                   )

                << std::setw(10)
                << details.listener.pid

                << std::setw(24)
                << process_name(
                       details
                   )

                << sanitize_terminal_text(
                       details.listener.address
                   )

                << '\n';
        }
    }

    void print_port_details(
        std::ostream& output,
        const std::vector<PortDetails>& details
    ) {
        for (
            std::size_t index = 0;
            index < details.size();
            ++index) {
            if (index != 0) {
                output << '\n';
            }

            print_single_port_details(
                output,
                details[index]
            );
        }
    }

    void print_kill_result(
        std::ostream& output,
        const KillResult& result
    ) {
        const auto name =
            sanitize_terminal_text(
                result.process_name
                    .value_or("process")
            );

        const bool forced =
            result.mode == TerminationMode::force;

        output
            << (forced
                    ? "Force-stopped "
                    : "Stopped ")
            << name
            << " (PID "
            << result.pid
            << ')';

        if (result.port) {
            output
                << " listening on :"
                << *result.port;
        }

        output << ".\n";
    }

    void print_force_hint(
        std::ostream& output,
        const KillResult& result
    ) {
        const auto name =
            sanitize_terminal_text(
                result.process_name
                    .value_or("process")
            );

        output
            << name
            << " (PID "
            << result.pid
            << ") did not exit after SIGTERM.\n"
            << "Use `devdock kill ";

        if (result.port) {
            output
                << *result.port;
        } else {
            output
                << "--pid "
                << result.pid;
        }

        output
            << " --force` to send SIGKILL.\n";
    }

} // namespace devdock