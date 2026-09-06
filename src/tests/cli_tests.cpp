#include "tests/test_support.hpp"

#include "cli/arguments.hpp"
#include "cli/formatter.hpp"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace {

    using namespace devdock;
    using namespace devdock::test_support;

    ParsedArguments parse(
        std::initializer_list<
            std::string_view>
            values
    ) {
        return parse_arguments(
            std::vector<std::string_view>{
                values
            }
        );
    }

    KillResult make_kill_result(
        TerminationMode mode,
        StopOutcome outcome,
        std::optional<std::uint16_t> port
    ) {
        return KillResult{
            .pid = 42,
            .port = port,
            .process_name = "node",
            .outcome = outcome,
            .mode = mode,
        };
    }

    std::string timeout_report(
        TerminationMode mode,
        std::optional<std::uint16_t> port
    ) {
        std::ostringstream output;

        print_stop_timeout(
            output,
            make_kill_result(
                mode,
                StopOutcome::still_running,
                port
            )
        );

        return output.str();
    }

    void graceful_timeout_advises_force_for_a_port() {
        const auto report =
            timeout_report(
                TerminationMode::graceful,
                5173
            );

        CHECK(
            report ==
            "node (PID 42) did not exit after SIGTERM.\n"
            "Use `devdock kill 5173 --force` to send SIGKILL.\n"
        );
    }

    void graceful_timeout_advises_force_for_a_pid() {
        const auto report =
            timeout_report(
                TerminationMode::graceful,
                std::nullopt
            );

        CHECK(
            report ==
            "node (PID 42) did not exit after SIGTERM.\n"
            "Use `devdock kill --pid 42 --force` to send SIGKILL.\n"
        );
    }

    void forced_timeout_reports_sigkill_and_advises_no_escalation() {
        const auto report =
            timeout_report(
                TerminationMode::force,
                5173
            );

        CHECK(
            report ==
            "node (PID 42) did not exit after SIGKILL.\n"
            "The process is most likely blocked in the kernel; "
            "check its state with `ps -o stat= -p 42`.\n"
        );

        CHECK(
            !report.contains("--force")
        );

        CHECK(
            !report.contains("SIGTERM")
        );
    }

    void stopped_result_names_the_process() {
        std::ostringstream output;

        print_kill_result(
            output,
            make_kill_result(
                TerminationMode::force,
                StopOutcome::stopped,
                5173
            )
        );

        CHECK(
            output.str() ==
            "Force-stopped node (PID 42) listening on :5173.\n"
        );
    }

    void ambiguous_target_error_advises_killing_by_pid() {
        std::ostringstream output;

        print_error(
            output,
            Error{
                .code = ErrorCode::ambiguous_target,
                .message = "Multiple processes are listening on port 5173.",
            }
        );

        CHECK(
            output.str() ==
            "devdock: Multiple processes are listening on port 5173.\n"
            "Use `devdock kill --pid <pid>` to stop one of them.\n"
        );
    }

    void ordinary_error_carries_no_advice() {
        std::ostringstream output;

        print_error(
            output,
            Error{
                .code = ErrorCode::not_found,
                .message = "No process is listening on port 5173.",
            }
        );

        CHECK(
            output.str() ==
            "devdock: No process is listening on port 5173.\n"
        );
    }

    void parses_ports() {
        const auto result =
            parse({"ports"});

        CHECK(result.has_value());

        CHECK(
            std::holds_alternative<
                ListPortsCommand>(*result)
        );
    }

    void parses_inspect_port() {
        const auto result =
            parse({"port", "5173"});

        CHECK(result.has_value());

        const auto* command =
            std::get_if<
                InspectPortCommand>(&*result);

        REQUIRE(command != nullptr);
        CHECK(command->port == 5173);
    }

    void parses_graceful_kill() {
        const auto result =
            parse({"kill", "5173"});

        CHECK(result.has_value());

        const auto* command =
            std::get_if<
                KillPortCommand>(&*result);

        REQUIRE(command != nullptr);

        CHECK(
            command->mode == TerminationMode::graceful
        );
    }

    void parses_force_kill() {
        const auto result =
            parse({"kill", "5173", "--force"});

        CHECK(result.has_value());

        const auto* command =
            std::get_if<
                KillPortCommand>(&*result);

        REQUIRE(command != nullptr);

        CHECK(
            command->mode == TerminationMode::force
        );
    }

    void parses_kill_by_pid() {
        const auto result =
            parse({"kill", "--pid", "42"});

        CHECK(result.has_value());

        const auto* command =
            std::get_if<
                KillPidCommand>(&*result);

        REQUIRE(command != nullptr);
        CHECK(command->pid == 42);
    }

    void rejects_invalid_port() {
        const auto result =
            parse({"port", "70000"});

        CHECK(!result.has_value());
    }

    void rejects_invalid_pid() {
        const auto result =
            parse({"kill", "--pid", "-1"});

        CHECK(!result.has_value());
    }

    std::string render_process_details(const std::optional<std::vector<std::string>>& arguments) {
        const PortDetails details{
            .listener = {.port = 5173, .protocol = Protocol::tcp, .address = "127.0.0.1", .pid = 42},
            .process = Process{
                .identity = {.pid = 42, .owner_user_id = 501, .start_time_token = 100},
                .name = "node",
                .arguments = arguments,
                .working_directory = std::nullopt,
            },
        };
        std::ostringstream output;
        print_port_details(output, {details});
        return output.str();
    }

    void formats_command_arguments_without_losing_empty_strings() {
        const auto output = render_process_details(std::vector<std::string>{"node", "", "two words", "it's", "\"quoted\"", ""});
        CHECK(output.contains("Command    node '' 'two words' 'it'\\''s' '\"quoted\"' ''\n"));
    }

    void formats_ordinary_command_arguments() {
        const auto output = render_process_details(std::vector<std::string>{"node", "vite"});
        CHECK(output.contains("Command    node vite\n"));
    }

    void formats_unavailable_command_arguments() {
        const auto output = render_process_details(std::nullopt);
        CHECK(output.contains("Command    <unavailable>\n"));
    }

    void escapes_terminal_controls_in_command_arguments() {
        const auto output = render_process_details(std::vector<std::string>{"node", "bad\x1B[2J\n"});
        CHECK(output.contains("Command    node bad\\x1B[2J\\n\n"));
    }

} // namespace

int main() {
    return run_suite(
        "CLI",
        {
            parses_ports,
            parses_inspect_port,
            parses_graceful_kill,
            parses_force_kill,
            parses_kill_by_pid,
            rejects_invalid_port,
            rejects_invalid_pid,
            ambiguous_target_error_advises_killing_by_pid,
            ordinary_error_carries_no_advice,
            graceful_timeout_advises_force_for_a_port,
            graceful_timeout_advises_force_for_a_pid,
            forced_timeout_reports_sigkill_and_advises_no_escalation,
            stopped_result_names_the_process,
            formats_command_arguments_without_losing_empty_strings,
            formats_ordinary_command_arguments,
            formats_unavailable_command_arguments,
            escapes_terminal_controls_in_command_arguments,
        }
    );
}
