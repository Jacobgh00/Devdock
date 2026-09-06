#include "ports/port_inspector.hpp"
#include "ports/process_controller.hpp"
#include "ports/process_inspector.hpp"
#include "use_cases/inspect_port.hpp"
#include "use_cases/kill_process.hpp"
#include "use_cases/list_ports.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

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

    class FakePortInspector final
        : public PortInspector {
    public:
        std::vector<
            std::vector<ListeningPort>>
            snapshots;

        std::optional<Error> error;
        std::size_t error_on_call = 0;

        mutable std::size_t calls = 0;

        Result<std::vector<ListeningPort>>
        listening_ports() const override {
            if (error && calls >= error_on_call) {
                return std::unexpected(
                    *error
                );
            }

            if (snapshots.empty()) {
                return std::vector<
                    ListeningPort>{};
            }

            const auto index =
                std::min(
                    calls,
                    snapshots.size() - 1
                );

            ++calls;

            return snapshots.at(index);
        }
    };

    class FakeProcessInspector final
        : public ProcessInspector {
    public:
        std::unordered_map<
            ProcessId,
            Process>
            processes;

        [[nodiscard]] Result<Process> inspect(
            ProcessId pid
        ) const override {
            const auto found =
                processes.find(pid);

            if (
                found != processes.end()
            ) {
                return found->second;
            }

            return std::unexpected(
                Error{
                    .code = ErrorCode::not_found,
                    .message = "Process not found.",
                }
            );
        }
    };

    class FakeProcessController final
        : public ProcessController {
    public:
        struct StopCall {
            ProcessIdentity identity;
            TerminationMode mode;
        };

        mutable std::vector<StopCall>
            calls;

        bool stopped = true;

        Result<bool> stop(
            const ProcessIdentity& identity,
            TerminationMode mode,
            std::chrono::milliseconds /*timeout*/
        ) const override {
            calls.push_back(
                StopCall{
                    .identity = identity,
                    .mode = mode,
                }
            );

            return stopped;
        }
    };

    Process make_process(
        ProcessId pid,
        std::string name = "node"
    ) {
        return Process{
            .identity =
                ProcessIdentity{
                    .pid = pid,
                    .owner_user_id = 501,
                    .start_time_token =
                        123456,
                },
            .name = std::move(name),
            .arguments = std::vector<std::string>{"node", "vite"},
            .working_directory =
                "/tmp/project",
        };
    }

    ListeningPort make_listener(
        std::uint16_t port,
        ProcessId pid
    ) {
        return ListeningPort{
            .port = port,
            .protocol = Protocol::tcp,
            .address = "127.0.0.1",
            .pid = pid,
        };
    }

    void list_ports_adds_process_metadata() {
        FakePortInspector ports;

        ports.snapshots = {
            {
                make_listener(
                    5173,
                    42
                ),
            },
        };

        FakeProcessInspector processes;

        processes.processes.emplace(
            42,
            make_process(42)
        );

        const auto result =
            ListPorts{
                ports,
                processes
            }
                .execute();

        CHECK(result.has_value());

        const bool has_single_entry =
            result && result->size() == 1;

        CHECK(has_single_entry);

        if (!has_single_entry) {
            return;
        }

        const auto& process =
            result->front()
                .process;

        CHECK(process.has_value());

        if (process) {
            CHECK(process->name == "node");
        }
    }

    void list_ports_keeps_listener_without_process_metadata() {
        FakePortInspector ports;

        ports.snapshots = {
            {
                make_listener(
                    6379,
                    99
                ),
            },
        };

        FakeProcessInspector processes;

        const auto result =
            ListPorts{
                ports,
                processes
            }
                .execute();

        CHECK(result.has_value());
        CHECK(result->size() == 1);

        CHECK(
            !result->front()
                 .process
                 .has_value()
        );
    }

    void inspect_port_returns_not_found() {
        FakePortInspector ports;

        ports.snapshots = {{}};

        FakeProcessInspector processes;

        const auto result =
            InspectPort{
                ports,
                processes
            }
                .execute(
                    5173
                );

        CHECK(!result.has_value());

        CHECK(
            result.error().code == ErrorCode::not_found
        );
    }

    void kill_by_port_revalidates_owner() {
        FakePortInspector ports;

        ports.snapshots = {
            {
                make_listener(
                    5173,
                    42
                ),
            },
            {
                make_listener(
                    5173,
                    43
                ),
            },
        };

        FakeProcessInspector processes;

        processes.processes.emplace(
            42,
            make_process(42)
        );

        FakeProcessController controller;

        const auto result =
            KillProcess{
                ports,
                processes,
                controller
            }
                .by_port(
                    5173,
                    TerminationMode::graceful
                );

        CHECK(!result.has_value());

        CHECK(
            result.error().code == ErrorCode::target_changed
        );

        CHECK(
            controller.calls.empty()
        );
    }

    void kill_by_port_passes_process_identity() {
        FakePortInspector ports;

        ports.snapshots = {
            {
                make_listener(
                    5173,
                    42
                ),
            },
            {
                make_listener(
                    5173,
                    42
                ),
            },
        };

        FakeProcessInspector processes;

        const auto process =
            make_process(42);

        processes.processes.emplace(
            42,
            process
        );

        FakeProcessController controller;

        const auto result =
            KillProcess{
                ports,
                processes,
                controller
            }
                .by_port(
                    5173,
                    TerminationMode::graceful
                );

        CHECK(result.has_value());

        CHECK(
            controller.calls.size() == 1
        );

        CHECK(
            controller.calls
                .front()
                .identity == process.identity
        );
    }

    void kill_by_port_refuses_ambiguous_owner() {
        FakePortInspector ports;

        ports.snapshots = {
            {
                make_listener(
                    5173,
                    42
                ),
                ListeningPort{
                    .port = 5173,
                    .protocol =
                        Protocol::tcp6,
                    .address = "::1",
                    .pid = 43,
                },
            },
        };

        FakeProcessInspector processes;
        FakeProcessController controller;

        const auto result =
            KillProcess{
                ports,
                processes,
                controller
            }
                .by_port(
                    5173,
                    TerminationMode::graceful
                );

        CHECK(!result.has_value());

        CHECK(
            result.error().code == ErrorCode::ambiguous_target
        );

        CHECK(
            controller.calls.empty()
        );
    }

    void kill_by_port_refuses_owner_that_becomes_ambiguous() {
        FakePortInspector ports;
        ports.snapshots = {
            {make_listener(5173, 42)},
            {
                make_listener(5173, 42),
                ListeningPort{
                    .port = 5173,
                    .protocol = Protocol::tcp6,
                    .address = "::1",
                    .pid = 43,
                },
            },
        };

        FakeProcessInspector processes;
        processes.processes.emplace(42, make_process(42));
        FakeProcessController controller;

        const auto result = KillProcess{ports, processes, controller}
                                .by_port(5173, TerminationMode::graceful);

        CHECK(!result.has_value());

        if (!result) {
            CHECK(result.error().code == ErrorCode::ambiguous_target);
        }

        CHECK(controller.calls.empty());
    }

    void kill_by_port_refuses_owner_that_disappears() {
        FakePortInspector ports;
        ports.snapshots = {{make_listener(5173, 42)}, {}};

        FakeProcessInspector processes;
        processes.processes.emplace(42, make_process(42));
        FakeProcessController controller;

        const auto result = KillProcess{ports, processes, controller}
                                .by_port(5173, TerminationMode::graceful);

        CHECK(!result.has_value());

        if (!result) {
            CHECK(result.error().code == ErrorCode::target_changed);
        }

        CHECK(controller.calls.empty());
    }

    void kill_by_port_accepts_multiple_listeners_for_one_owner() {
        FakePortInspector ports;
        const std::vector<ListeningPort> listeners = {
            make_listener(5173, 42),
            ListeningPort{
                .port = 5173,
                .protocol = Protocol::tcp6,
                .address = "::1",
                .pid = 42,
            },
            make_listener(6379, 99),
        };
        ports.snapshots = {listeners, listeners};

        FakeProcessInspector processes;
        const auto process = make_process(42);
        processes.processes.emplace(42, process);
        FakeProcessController controller;

        const auto result = KillProcess{ports, processes, controller}
                                .by_port(5173, TerminationMode::graceful);

        CHECK(result.has_value());
        CHECK(controller.calls.size() == 1);

        if (controller.calls.size() == 1) {
            CHECK(controller.calls.front().identity == process.identity);
        }
    }

    void kill_by_port_preserves_scan_errors() {
        for (const auto code : {ErrorCode::not_found, ErrorCode::permission_denied, ErrorCode::system_error}) {
            for (const std::size_t error_on_call : {0U, 1U}) {
                FakePortInspector ports;
                ports.snapshots = {{make_listener(5173, 42)}};
                ports.error = Error{.code = code, .message = "Listener scan failed."};
                ports.error_on_call = error_on_call;

                FakeProcessInspector processes;
                processes.processes.emplace(42, make_process(42));
                FakeProcessController controller;

                const auto result = KillProcess{ports, processes, controller}
                                        .by_port(5173, TerminationMode::graceful);

                CHECK(!result.has_value());

                if (!result) {
                    CHECK(result.error().code == code);
                    CHECK(result.error().message == "Listener scan failed.");
                }

                CHECK(controller.calls.empty());
            }
        }
    }

    void kill_by_pid_preserves_force_mode() {
        FakePortInspector ports;
        FakeProcessInspector processes;

        const auto process =
            make_process(42);

        processes.processes.emplace(
            42,
            process
        );

        FakeProcessController controller;

        const auto result =
            KillProcess{
                ports,
                processes,
                controller
            }
                .by_pid(
                    42,
                    TerminationMode::force
                );

        CHECK(result.has_value());

        CHECK(
            controller.calls.size() == 1
        );

        CHECK(
            controller.calls
                .front()
                .mode == TerminationMode::force
        );
    }

} // namespace

int main() {
    try {
        list_ports_adds_process_metadata();

        list_ports_keeps_listener_without_process_metadata();

        inspect_port_returns_not_found();

        kill_by_port_revalidates_owner();

        kill_by_port_passes_process_identity();

        kill_by_port_refuses_ambiguous_owner();

        kill_by_port_refuses_owner_that_becomes_ambiguous();

        kill_by_port_refuses_owner_that_disappears();

        kill_by_port_accepts_multiple_listeners_for_one_owner();

        kill_by_port_preserves_scan_errors();

        kill_by_pid_preserves_force_mode();

        if (failure_count() != 0) {
            std::cerr
                << failure_count()
                << " test(s) failed\n";

            return EXIT_FAILURE;
        }

        std::cout
            << "All application tests passed\n";

        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr
            << "Unexpected exception: "
            << error.what()
            << '\n';

        return 1;
    }
}
