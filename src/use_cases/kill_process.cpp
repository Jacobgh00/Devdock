#include "use_cases/kill_process.hpp"

#include "use_cases/listener_lookup.hpp"

#include <chrono>

namespace devdock {

    namespace {
        constexpr auto shutdown_timeout =
            std::chrono::seconds{2};

    } // namespace

    Result<KillResult> KillProcess::by_port(
        std::uint16_t port,
        TerminationMode mode
    ) const {
        auto scan = port_inspector_.listening_ports();

        if (!scan) {
            return std::unexpected(scan.error());
        }

        auto owner =
            unique_owner_of(*scan, port);

        if (!owner) {
            return std::unexpected(
                owner.error()
            );
        }

        auto process =
            process_inspector_.inspect(
                *owner
            );

        if (!process) {
            return std::unexpected(
                process.error()
            );
        }

        auto confirmation =
            confirm_owner(
                port,
                process->identity
            );

        if (!confirmation) {
            return std::unexpected(
                confirmation.error()
            );
        }

        return stop_process(
            *process,
            port,
            mode
        );
    }

    Result<KillResult> KillProcess::by_pid(
        ProcessId pid,
        TerminationMode mode
    ) const {
        if (pid <= 0) {
            return std::unexpected(
                Error{
                    .code = ErrorCode::invalid_argument,
                    .message = "PID must be greater than zero.",
                }
            );
        }

        auto process =
            process_inspector_.inspect(pid);

        if (!process) {
            return std::unexpected(
                process.error()
            );
        }

        return stop_process(
            *process,
            std::nullopt,
            mode
        );
    }

    Result<void> KillProcess::confirm_owner(
        std::uint16_t port,
        const ProcessIdentity& identity
    ) const {
        auto scan =
            port_inspector_.listening_ports();

        if (!scan) {
            return std::unexpected(
                scan.error()
            );
        }

        const auto owner = unique_owner_of(*scan, port);

        if (!owner && owner.error().code != ErrorCode::not_found) {
            return std::unexpected(owner.error());
        }

        if (owner && *owner == identity.pid) {
            return {};
        }

        return std::unexpected(
            Error{
                .code = ErrorCode::target_changed,
                .message = "The process owning port " + std::to_string(port) + " changed before it could be stopped. "
                                                                               "Try the command again.",
            }
        );
    }

    Result<KillResult>
    KillProcess::stop_process(
        const Process& process,
        std::optional<std::uint16_t> port,
        TerminationMode mode
    ) const {
        auto outcome =
            process_controller_.stop(
                process.identity,
                mode,
                shutdown_timeout
            );

        if (!outcome) {
            return std::unexpected(
                outcome.error()
            );
        }

        return KillResult{
            .pid = process.identity.pid,
            .port = port,
            .process_name = process.name,
            .outcome = *outcome,
            .mode = mode,
        };
    }

}
