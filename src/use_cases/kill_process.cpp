#include "use_cases/kill_process.hpp"

#include <chrono>
#include <set>

namespace devdock {

    namespace {
        constexpr auto shutdown_timeout =
            std::chrono::seconds{2};

    } // namespace

    Result<KillResult> KillProcess::by_port(
        std::uint16_t port,
        TerminationMode mode
    ) const {
        auto listeners = port_inspector_.listening_ports();

        if (!listeners) {
            return std::unexpected(listeners.error());
        }

        auto owner =
            find_unique_owner(*listeners, port);

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

    Result<ProcessId>
    KillProcess::find_unique_owner(
        const std::vector<ListeningPort>& listeners,
        std::uint16_t port
    ) {
        std::set<ProcessId> owners;

        for (const auto& listener : listeners) {
            if (listener.port == port) {
                owners.insert(
                    listener.pid
                );
            }
        }

        if (owners.empty()) {
            return std::unexpected(
                Error{
                    .code = ErrorCode::not_found,
                    .message = "No process is listening on port " + std::to_string(port) + ".",
                }
            );
        }

        if (owners.size() > 1) {
            return std::unexpected(
                Error{
                    .code = ErrorCode::ambiguous_target,
                    .message = "Multiple processes are listening on port " + std::to_string(port) + "; use `devdock kill --pid <pid>` instead.",
                }
            );
        }

        return *owners.begin();
    }

    Result<void> KillProcess::confirm_owner(
        std::uint16_t port,
        const ProcessIdentity& identity
    ) const {
        auto listeners =
            port_inspector_.listening_ports();

        if (!listeners) {
            return std::unexpected(
                listeners.error()
            );
        }

        const auto owner = find_unique_owner(*listeners, port);

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
        auto stopped =
            process_controller_.stop(
                process.identity,
                mode,
                shutdown_timeout
            );

        if (!stopped) {
            return std::unexpected(
                stopped.error()
            );
        }

        return KillResult{
            .pid = process.identity.pid,
            .port = port,
            .process_name = process.name,
            .status =
                *stopped
                    ? KillStatus::stopped
                    : KillStatus::still_running,
            .mode = mode,
        };
    }

}
