#include "tests/test_support.hpp"

#include "tests/owned_resources.hpp"

#include "platform/mac/mac_port_inspector.hpp"
#include "platform/mac/mac_process_controller.hpp"
#include "platform/mac/mac_process_identity.hpp"
#include "platform/mac/mac_process_inspector.hpp"

#include <arpa/inet.h>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <exception>
#include <fcntl.h>
#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {

    using namespace devdock;
    using namespace devdock::test_support;

    // Forks a child that waits until it is signalled.
    pid_t fork_waiting_child() {
        const pid_t child = ::fork();

        if (child == 0) {
            for (;;) {
                ::pause();
            }
        }

        return child;
    }

    void an_owned_child_is_stopped_and_reaped_on_scope_exit() {
        if (::geteuid() == 0) {
            std::cout
                << "Skipping destructive child ownership test as root\n";

            return;
        }

        pid_t pid = 0;

        {
            OwnedChild child{fork_waiting_child()};

            REQUIRE(child.pid() > 0);

            pid = child.pid();
        }

        errno = 0;

        CHECK(
            ::waitpid(pid, nullptr, WNOHANG) == -1
        );

        CHECK(errno == ECHILD);
    }

    void an_owned_descriptor_is_closed_on_scope_exit() {
        int raw = -1;

        {
            OwnedDescriptor descriptor{
                ::socket(AF_INET, SOCK_STREAM, 0)
            };

            REQUIRE(descriptor.get() >= 0);

            raw = descriptor.get();
        }

        errno = 0;

        // dup rather than fcntl: it reports a closed descriptor the same way
        // without a vararg call.
        CHECK(::dup(raw) == -1);

        CHECK(errno == EBADF);
    }

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
        const OwnedDescriptor socket_fd{
            ::socket(
                AF_INET,
                SOCK_STREAM,
                0
            )
        };

        REQUIRE(socket_fd.get() >= 0);

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
                socket_fd.get(),
                reinterpret_cast<
                    sockaddr*>(&address),
                sizeof(address)
            ) == 0
        );

        CHECK(
            ::listen(
                socket_fd.get(),
                1
            ) == 0
        );

        socklen_t length =
            sizeof(address);

        CHECK(
            ::getsockname(
                socket_fd.get(),
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

        REQUIRE(scan.has_value());

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

        OwnedChild child{fork_waiting_child()};

        REQUIRE(child.pid() > 0);

        MacProcessInspector inspector;

        const auto process =
            inspector.inspect(
                child.pid()
            );

        REQUIRE(process.has_value());

        MacProcessController controller;

        const auto stopped =
            controller.stop(
                process->identity,
                TerminationMode::graceful,
                std::chrono::seconds{1}
            );

        REQUIRE(stopped.has_value());

        CHECK(*stopped == StopOutcome::stopped);

        /*
         * The controller recognizes SZOMB as stopped.
         * OwnedChild reaps the zombie, and stops and reaps
         * the child on every failure path above.
         */
    }

} // namespace

int main() {
    return run_suite(
        "macOS platform",
        {
            an_owned_child_is_stopped_and_reaped_on_scope_exit,
            an_owned_descriptor_is_closed_on_scope_exit,
            identity_reader_reads_current_process,
            process_inspector_reads_current_process,
            port_inspector_finds_current_listener,
            port_inspector_reports_scan_coverage,
            controller_refuses_changed_identity,
            controller_terminates_child,
        }
    );
}
