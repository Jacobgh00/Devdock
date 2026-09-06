#pragma once

#include "domain/process_identity.hpp"
#include "ports/process_controller.hpp"

#include <cstdint>
#include <expected>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace devdock {

    struct HelpCommand {};

    struct ListPortsCommand {};

    struct InspectPortCommand {
        std::uint16_t port;
    };

    struct KillPortCommand {
        std::uint16_t port;
        TerminationMode mode;
    };

    struct KillPidCommand {
        ProcessId pid;
        TerminationMode mode;
    };

    using Command =
        std::variant<
            HelpCommand,
            ListPortsCommand,
            InspectPortCommand,
            KillPortCommand,
            KillPidCommand>;

    struct ArgumentError {
        std::string message;
    };

    using ParsedArguments =
        std::expected<
            Command,
            ArgumentError>;

    ParsedArguments parse_arguments(
        const std::vector<std::string_view>&
            arguments
    );

} // namespace devdock