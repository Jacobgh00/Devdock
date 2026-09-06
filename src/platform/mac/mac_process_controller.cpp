#include "platform/mac/mac_process_controller.hpp"

#include "platform/mac/mac_process_identity.hpp"
#include "platform/mac/mac_process_safety.hpp"

#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstring>
#include <thread>

namespace devdock {

    namespace {

        Error signal_error(
            ProcessId pid
        ) {
            const auto code =
                errno == ESRCH
                    ? ErrorCode::not_found
                : (
                      errno == EPERM || errno == EACCES
                  )
                    ? ErrorCode::permission_denied
                    : ErrorCode::termination_failed;

            return Error{
                .code = code,
                .message = "Unable to signal process for PID " + std::to_string(pid) + ": " + std::strerror(errno),
            };
        }

        Result<void> verify_identity(
            const ProcessIdentity& expected
        ) {
            auto current =
                mac_detail::read_process_identity(
                    expected.pid
                );

            if (!current) {
                return std::unexpected(
                    current.error()
                );
            }

            if (*current != expected) {
                return std::unexpected(
                    Error{
                        .code = ErrorCode::target_changed,
                        .message = "The target process changed before "
                                   "it could be stopped. "
                                   "Try the command again.",
                    }
                );
            }

            return {};
        }

        // true when the process was already gone when the signal was sent.
        Result<bool> send_signal(
            ProcessId pid,
            TerminationMode mode
        ) {
            const int signal =
                mode == TerminationMode::force
                    ? SIGKILL
                    : SIGTERM;

            errno = 0;

            if (::kill(pid, signal) == 0) {
                return false;
            }

            if (errno == ESRCH) {
                return true;
            }

            return std::unexpected(
                signal_error(pid)
            );
        }

        Result<bool>
        original_process_has_stopped(
            const ProcessIdentity& expected
        ) {
            auto snapshot =
                mac_detail::read_process_snapshot(
                    expected.pid
                );

            if (!snapshot) {
                if (
                    snapshot.error().code == ErrorCode::not_found
                ) {
                    return true;
                }

                return std::unexpected(
                    snapshot.error()
                );
            }

            if (
                snapshot->identity != expected
            ) {
                /*
                 * The original process disappeared and
                 * macOS reused the PID.
                 */
                return true;
            }

            if (snapshot->zombie) {
                return true;
            }

            return false;
        }

        Result<bool> wait_for_exit(
            const ProcessIdentity& identity,
            std::chrono::milliseconds timeout
        ) {
            const auto deadline =
                std::chrono::steady_clock::now() + timeout;

            while (
                std::chrono::steady_clock::now() < deadline) {
                auto stopped =
                    original_process_has_stopped(
                        identity
                    );

                if (
                    !stopped || *stopped
                ) {
                    return stopped;
                }

                std::this_thread::sleep_for(
                    std::chrono::milliseconds{50}
                );
            }

            return original_process_has_stopped(
                identity
            );
        }

    } // namespace

    Result<StopOutcome>
    MacProcessController::stop(
        const ProcessIdentity& identity,
        TerminationMode mode,
        std::chrono::milliseconds timeout
    ) const {
        auto safety =
            validate_process_target(
                identity
            );

        if (!safety) {
            return std::unexpected(
                safety.error()
            );
        }

        /*
         * macOS does not expose a public pidfd-like
         * stable process handle.
         *
         * Revalidate PID + UID + process start time
         * immediately before signalling.
         */
        auto verified =
            verify_identity(identity);

        if (!verified) {
            return std::unexpected(
                verified.error()
            );
        }

        auto already_stopped =
            send_signal(
                identity.pid,
                mode
            );

        if (!already_stopped) {
            return std::unexpected(
                already_stopped.error()
            );
        }

        if (*already_stopped) {
            return StopOutcome::stopped;
        }

        const auto stopped =
            wait_for_exit(
                identity,
                timeout
            );

        if (!stopped) {
            return std::unexpected(
                stopped.error()
            );
        }

        return *stopped
                   ? StopOutcome::stopped
                   : StopOutcome::still_running;
    }

} // namespace devdock