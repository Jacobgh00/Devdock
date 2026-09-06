#include "cli/cli.hpp"

#include "cli/arguments.hpp"
#include "cli/formatter.hpp"
#include "use_cases/inspect_port.hpp"
#include "use_cases/kill_process.hpp"
#include "use_cases/list_ports.hpp"

#include <iostream>
#include <string_view>
#include <variant>
#include <vector>

namespace devdock {

    namespace {

        constexpr int success = 0;
        constexpr int general_failure = 1;
        constexpr int invalid_arguments = 2;
        constexpr int target_not_found = 3;
        constexpr int permission_denied = 4;

        int exit_code(
            const Error& error
        ) {
            switch (error.code) {
            case ErrorCode::invalid_argument:
                return invalid_arguments;

            case ErrorCode::not_found:
                return target_not_found;

            case ErrorCode::permission_denied:
            case ErrorCode::protected_process:
                return permission_denied;

            default:
                return general_failure;
            }
        }

        int report_error(
            const Error& error
        ) {
            print_error(
                std::cerr,
                error
            );

            return exit_code(error);
        }

        int execute_list_ports(
            const PortInspector& port_inspector,
            const ProcessInspector& process_inspector
        ) {
            auto result =
                ListPorts{
                    port_inspector,
                    process_inspector
                }
                    .execute();

            if (!result) {
                return report_error(
                    result.error()
                );
            }

            print_ports(
                std::cout,
                *result
            );

            return success;
        }

        int execute_inspect_port(
            const InspectPortCommand& command,
            const PortInspector& port_inspector,
            const ProcessInspector& process_inspector
        ) {
            auto result =
                InspectPort{
                    port_inspector,
                    process_inspector
                }
                    .execute(
                        command.port
                    );

            if (!result) {
                return report_error(
                    result.error()
                );
            }

            print_port_details(
                std::cout,
                *result
            );

            return success;
        }

        int report_kill_result(
            const KillResult& result
        ) {
            if (
                result.status == KillStatus::still_running
            ) {
                print_force_hint(
                    std::cerr,
                    result
                );

                return general_failure;
            }

            print_kill_result(
                std::cout,
                result
            );

            return success;
        }

        int execute_kill_port(
            const KillPortCommand& command,
            const PortInspector& port_inspector,
            const ProcessInspector& process_inspector,
            const ProcessController& process_controller
        ) {
            auto result =
                KillProcess{
                    port_inspector,
                    process_inspector,
                    process_controller,
                }
                    .by_port(
                        command.port,
                        command.mode
                    );

            if (!result) {
                return report_error(
                    result.error()
                );
            }

            return report_kill_result(
                *result
            );
        }

        int execute_kill_pid(
            const KillPidCommand& command,
            const PortInspector& port_inspector,
            const ProcessInspector& process_inspector,
            const ProcessController& process_controller
        ) {
            auto result =
                KillProcess{
                    port_inspector,
                    process_inspector,
                    process_controller,
                }
                    .by_pid(
                        command.pid,
                        command.mode
                    );

            if (!result) {
                return report_error(
                    result.error()
                );
            }

            return report_kill_result(
                *result
            );
        }

        std::vector<std::string_view>
        command_arguments(
            std::span<char* const> arguments_in
        ) {
            std::vector<std::string_view>
                arguments;

            if (arguments_in.empty()) {
                return arguments;
            }

            const auto without_program_name =
                arguments_in.subspan(1);

            arguments.reserve(
                without_program_name.size()
            );

            for (
                char* const argument : without_program_name) {
                arguments.emplace_back(argument);
            }

            return arguments;
        }

    } // namespace

    int run_cli(
        std::span<char* const> arguments,
        const PortInspector& port_inspector,
        const ProcessInspector& process_inspector,
        const ProcessController& process_controller
    ) {
        const auto command =
            parse_arguments(
                command_arguments(
                    arguments
                )
            );

        if (!command) {
            print_argument_error(
                std::cerr,
                command.error().message
            );

            std::cerr << '\n';

            print_usage(
                std::cerr
            );

            return invalid_arguments;
        }

        if (
            std::holds_alternative<
                HelpCommand>(*command)
        ) {
            print_usage(
                std::cout
            );

            return success;
        }

        if (
            std::holds_alternative<
                ListPortsCommand>(*command)
        ) {
            return execute_list_ports(
                port_inspector,
                process_inspector
            );
        }

        if (
            const auto* inspect =
                std::get_if<
                    InspectPortCommand>(&*command)
        ) {
            return execute_inspect_port(
                *inspect,
                port_inspector,
                process_inspector
            );
        }

        if (
            const auto* kill_port =
                std::get_if<
                    KillPortCommand>(&*command)
        ) {
            return execute_kill_port(
                *kill_port,
                port_inspector,
                process_inspector,
                process_controller
            );
        }

        const auto& kill_pid =
            std::get<
                KillPidCommand>(*command);

        return execute_kill_pid(
            kill_pid,
            port_inspector,
            process_inspector,
            process_controller
        );
    }

} // namespace devdock