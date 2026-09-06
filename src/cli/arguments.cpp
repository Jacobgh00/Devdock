#include "cli/arguments.hpp"

#include <charconv>
#include <optional>

namespace devdock {

    namespace {

        template <typename Integer>
        std::optional<Integer> parse_integer(
            std::string_view text
        ) {
            Integer value{};

            const auto [end, error] =
                std::from_chars(
                    text.data(),
                    text.data() + text.size(),
                    value
                );

            if (
                error != std::errc{} || end != text.data() + text.size()
            ) {
                return std::nullopt;
            }

            return value;
        }

        std::optional<std::uint16_t>
        parse_port(
            std::string_view text
        ) {
            const auto value =
                parse_integer<unsigned int>(
                    text
                );

            if (
                !value || *value == 0 || *value > 65535
            ) {
                return std::nullopt;
            }

            return static_cast<
                std::uint16_t>(*value);
        }

        std::optional<ProcessId>
        parse_pid(
            std::string_view text
        ) {
            const auto value =
                parse_integer<ProcessId>(
                    text
                );

            if (
                !value || *value <= 0
            ) {
                return std::nullopt;
            }

            return value;
        }

        TerminationMode termination_mode(
            bool force
        ) {
            return force
                       ? TerminationMode::force
                       : TerminationMode::graceful;
        }

        ParsedArguments parse_port_command(
            const std::vector<std::string_view>&
                arguments
        ) {
            if (arguments.size() != 2) {
                return std::unexpected(
                    ArgumentError{
                        "Usage: devdock port <port>"
                    }
                );
            }

            const auto port =
                parse_port(
                    arguments[1]
                );

            if (!port) {
                return std::unexpected(
                    ArgumentError{
                        "Invalid port: " + std::string{
                                               arguments[1]
                                           }
                    }
                );
            }

            return InspectPortCommand{
                *port
            };
        }

        ParsedArguments parse_kill_by_pid(
            const std::vector<std::string_view>&
                arguments
        ) {
            const bool force =
                arguments.size() == 4 && arguments[3] == "--force";

            if (
                arguments.size() != 3 && !force
            ) {
                return std::unexpected(
                    ArgumentError{
                        "Usage: devdock kill --pid <pid> [--force]"
                    }
                );
            }

            const auto pid =
                parse_pid(
                    arguments[2]
                );

            if (!pid) {
                return std::unexpected(
                    ArgumentError{
                        "Invalid PID: " + std::string{
                                              arguments[2]
                                          }
                    }
                );
            }

            return KillPidCommand{
                *pid,
                termination_mode(force)
            };
        }

        ParsedArguments parse_kill_by_port(
            const std::vector<std::string_view>&
                arguments
        ) {
            const bool force =
                arguments.size() == 3 && arguments[2] == "--force";

            if (
                arguments.size() != 2 && !force
            ) {
                return std::unexpected(
                    ArgumentError{
                        "Usage: devdock kill <port> [--force]"
                    }
                );
            }

            const auto port =
                parse_port(
                    arguments[1]
                );

            if (!port) {
                return std::unexpected(
                    ArgumentError{
                        "Invalid port: " + std::string{
                                               arguments[1]
                                           }
                    }
                );
            }

            return KillPortCommand{
                *port,
                termination_mode(force)
            };
        }

        ParsedArguments parse_kill_command(
            const std::vector<std::string_view>&
                arguments
        ) {
            if (arguments.size() < 2) {
                return std::unexpected(
                    ArgumentError{
                        "Usage: devdock kill <port> [--force] | "
                        "devdock kill --pid <pid> [--force]"
                    }
                );
            }

            if (arguments[1] == "--pid") {
                return parse_kill_by_pid(
                    arguments
                );
            }

            return parse_kill_by_port(
                arguments
            );
        }

    } // namespace

    ParsedArguments parse_arguments(
        const std::vector<std::string_view>&
            arguments
    ) {
        if (arguments.empty()) {
            return std::unexpected(
                ArgumentError{
                    "No command provided."
                }
            );
        }

        const auto command =
            arguments.front();

        if (
            command == "help" || command == "--help" || command == "-h"
        ) {
            return HelpCommand{};
        }

        if (
            command == "ports" && arguments.size() == 1
        ) {
            return ListPortsCommand{};
        }

        if (command == "port") {
            return parse_port_command(
                arguments
            );
        }

        if (command == "kill") {
            return parse_kill_command(
                arguments
            );
        }

        return std::unexpected(
            ArgumentError{
                "Unknown command: " + std::string{
                                          command
                                      }
            }
        );
    }

} // namespace devdock