#include "cli/arguments.hpp"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <string_view>
#include <variant>
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

        CHECK(command != nullptr);
        CHECK(command->port == 5173);
    }

    void parses_graceful_kill() {
        const auto result =
            parse({"kill", "5173"});

        CHECK(result.has_value());

        const auto* command =
            std::get_if<
                KillPortCommand>(&*result);

        CHECK(command != nullptr);

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

        CHECK(command != nullptr);

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

        CHECK(command != nullptr);
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

} // namespace

int main() {
    try {
        parses_ports();
        parses_inspect_port();
        parses_graceful_kill();
        parses_force_kill();
        parses_kill_by_pid();
        rejects_invalid_port();
        rejects_invalid_pid();

        if (failure_count() != 0) {
            std::cerr
                << failure_count()
                << " test(s) failed\n";

            return EXIT_FAILURE;
        }

        std::cout
            << "All CLI tests passed\n";

        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr
            << "Unexpected exception: "
            << error.what()
            << '\n';

        return 1;
    }
}
