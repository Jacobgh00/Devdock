#include "platform/mac/mac_port_inspector.hpp"
#include "platform/mac/mac_process_controller.hpp"
#include "platform/mac/mac_process_identity.hpp"
#include "platform/mac/mac_process_inspector.hpp"

#include <arpa/inet.h>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {

    using namespace devdock;

    int& failure_count() {
        static int failures = 0;

        return failures;
    }

#define CHECK(condition)             \
    do {                             \
        if (!(condition)) {          \
            std::cerr                \
                << __FILE__          \
                << ':'               \
                << __LINE__          \
                << " CHECK failed: " \
                << #condition        \
                << '\n';             \
            ++failure_count();       \
        }                            \
    } while (false)

    void identity_reader_reads_current_process() {
        const auto identity =
            mac_detail::read_process_identity(
                static_cast<ProcessId>(
                    ::getpid()
                )
            );

        CHECK(identity.has_value());

        CHECK(
            identity->pid == ::getpid()
        );

        CHECK(
            identity->owner_user_id == ::geteuid()
        );

        CHECK(
            identity->start_time_token != 0
        );
    }

    void process_inspector_reads_current_process() {
        MacProcessInspector inspector;

        const auto process =
            inspector.inspect(
                static_cast<ProcessId>(
                    ::getpid()
                )
            );

        CHECK(process.has_value());

        CHECK(
            process->identity.pid == ::getpid()
        );

        CHECK(
            !process->name.empty()
        );

        const bool has_arguments = process->arguments && !process->arguments->empty();
        CHECK(has_arguments);
    }

    void port_inspector_finds_current_listener() {
        const int socket_fd =
            ::socket(
                AF_INET,
                SOCK_STREAM,
                0
            );

        CHECK(socket_fd >= 0);

        sockaddr_in address{};

        address.sin_family =
            AF_INET;

        address.sin_addr.s_addr =
            htonl(
                INADDR_LOOPBACK
            );

        address.sin_port = 0;

        CHECK(
            ::bind(
                socket_fd,
                reinterpret_cast<
                    sockaddr*>(&address),
                sizeof(address)
            ) == 0
        );

        CHECK(
            ::listen(
                socket_fd,
                1
            ) == 0
        );

        socklen_t length =
            sizeof(address);

        CHECK(
            ::getsockname(
                socket_fd,
                reinterpret_cast<
                    sockaddr*>(&address),
                &length
            ) == 0
        );

        const auto port =
            ntohs(
                address.sin_port
            );

        MacPortInspector inspector;

        const auto scan =
            inspector.listening_ports();

        CHECK(
            scan.has_value()
        );

        if (!scan) {
            ::close(socket_fd);
            return;
        }

        bool found = false;

        for (
            const auto& listener : scan->listeners) {
            if (
                listener.port == port && listener.pid == ::getpid()
            ) {
                found = true;
                break;
            }
        }

        CHECK(found);

        ::close(socket_fd);
    }

    void port_inspector_reports_scan_coverage() {
        MacPortInspector inspector;

        const auto scan = inspector.listening_ports();

        CHECK(scan.has_value());

        if (!scan) {
            return;
        }

        CHECK(scan->scanned_processes > 0);

        CHECK(
            scan->uninspectable_processes <= scan->scanned_processes
        );

        // The test process is always inspectable by itself, so a scan can never
        // report every process as invisible.
        CHECK(
            scan->uninspectable_processes < scan->scanned_processes
        );
    }

    void check_stop_rejects_stale_identity(
        const ProcessIdentity& identity
    ) {
        auto stale_identity = identity;

        ++stale_identity
              .start_time_token;

        MacProcessController controller;

        const auto result =
            controller.stop(
                stale_identity,
                TerminationMode::graceful,
                std::chrono::milliseconds{
                    100
                }
            );

        CHECK(!result.has_value());

        if (!result) {
            CHECK(
                result.error().code == ErrorCode::target_changed
            );
        }
    }

    void controller_refuses_changed_identity() {
        if (::geteuid() == 0) {
            return;
        }

        const pid_t child =
            ::fork();

        CHECK(child >= 0);

        if (child < 0) {
            return;
        }

        if (child == 0) {
            for (;;) {
                ::pause();
            }
        }

        MacProcessInspector inspector;

        const auto process =
            inspector.inspect(
                child
            );

        CHECK(process.has_value());

        if (process) {
            check_stop_rejects_stale_identity(
                process->identity
            );
        }

        /*
         * The child is untouched by the rejected stop,
         * so terminate and reap it here.
         */
        ::kill(
            child,
            SIGKILL
        );

        ::waitpid(
            child,
            nullptr,
            0
        );
    }

    void controller_terminates_child() {
        if (::geteuid() == 0) {
            std::cout
                << "Skipping destructive controller test as root\n";

            return;
        }

        const pid_t child =
            ::fork();

        CHECK(child >= 0);

        if (child < 0) {
            return;
        }

        if (child == 0) {
            for (;;) {
                ::pause();
            }
        }

        MacProcessInspector inspector;

        const auto process =
            inspector.inspect(
                child
            );

        CHECK(process.has_value());

        if (!process) {
            ::kill(
                child,
                SIGKILL
            );

            ::waitpid(
                child,
                nullptr,
                0
            );

            return;
        }

        MacProcessController controller;

        const auto stopped =
            controller.stop(
                process->identity,
                TerminationMode::graceful,
                std::chrono::seconds{1}
            );

        CHECK(stopped.has_value());

        if (stopped) {
            CHECK(*stopped == StopOutcome::stopped);
        }

        /*
         * The controller recognizes SZOMB as stopped.
         * Reap the child afterwards.
         */
        ::waitpid(
            child,
            nullptr,
            0
        );
    }

} // namespace

int main() {
    try {
        identity_reader_reads_current_process();

        process_inspector_reads_current_process();

        port_inspector_finds_current_listener();

        port_inspector_reports_scan_coverage();

        controller_refuses_changed_identity();

        controller_terminates_child();

        if (failure_count() != 0) {
            std::cerr
                << failure_count()
                << " test(s) failed\n";

            return EXIT_FAILURE;
        }

        std::cout
            << "All macOS platform tests passed\n";

        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr
            << "Unexpected exception: "
            << error.what()
            << '\n';

        return 1;
    }
}
