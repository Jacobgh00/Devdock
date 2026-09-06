#include "platform/mac/mac_process_arguments.hpp"
#include "platform/mac/mac_process_inspector.hpp"

#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unistd.h>
#include <vector>

namespace {
    using namespace std::string_view_literals;

    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error{std::string{message}};
        }
    }

    std::vector<char> argument_buffer(int count, std::string_view payload) {
        std::vector<char> buffer(sizeof(count));
        std::memcpy(buffer.data(), &count, sizeof(count));
        buffer.insert(buffer.end(), payload.begin(), payload.end());
        return buffer;
    }

    void expect_arguments(int count, std::string_view payload, const std::vector<std::string>& expected) {
        const auto result = devdock::mac_detail::decode_process_arguments(argument_buffer(count, payload));
        require(result.has_value(), "Decoder rejected valid arguments.");
        require(*result == expected, "Decoded arguments differ from the expected exact strings.");
    }

    void decoder_preserves_empty_arguments_and_excludes_environment() {
        expect_arguments(4, "/bin/x\0\0node\0\0tail\0\0REVIEW_MARKER=synthetic_only\0"sv, {"node", "", "tail", ""});
        expect_arguments(3, "/bin/x\0\0\0tail\0\0REVIEW_MARKER=synthetic_only\0"sv, {"", "tail", ""});
        expect_arguments(2, "/bin/x\0\0\0\0REVIEW_MARKER=synthetic_only\0"sv, {"", ""});
    }

    void decoder_keeps_arguments_unformatted() {
        expect_arguments(4, "/bin/x\0\0node\0two words\0it's\0\"quoted\"\0"sv, {"node", "two words", "it's", "\"quoted\""});
    }

    void decoder_respects_executable_path_alignment() {
        expect_arguments(1, "/bin/ab\0node\0"sv, {"node"});
        expect_arguments(1, "/bin/abc\0\0\0\0\0\0\0\0node\0"sv, {"node"});
    }

    void expect_malformed(const std::vector<char>& buffer) {
        const auto result = devdock::mac_detail::decode_process_arguments(buffer);
        require(!result.has_value(), "Decoder accepted malformed or truncated arguments.");
        require(result.error().code == devdock::ErrorCode::system_error, "Malformed OS data did not report a system error.");
    }

    void decoder_rejects_malformed_input() {
        expect_malformed({});
        expect_malformed({'x'});
        expect_malformed(argument_buffer(1, ""sv));
        expect_malformed(argument_buffer(1, "/bin/x"sv));
        expect_malformed(argument_buffer(1, "/bin/x\0"sv));
        expect_malformed(argument_buffer(1, "/bin/x\0xnode\0"sv));
        expect_malformed(argument_buffer(1, "/bin/x\0\0unterminated"sv));
        expect_malformed(argument_buffer(2, "/bin/x\0\0node\0"sv));
        expect_malformed(argument_buffer(2, "/bin/x\0\0node\0unterminated"sv));
        expect_malformed(argument_buffer(0, "/bin/x\0\0node\0"sv));
        expect_malformed(argument_buffer(-1, "/bin/x\0\0node\0"sv));
        expect_malformed(argument_buffer(std::numeric_limits<int>::max(), "/bin/x\0\0node\0"sv));
    }

    int inspect_with_empty_argv0(const char* executable) {
        std::string empty;
        std::string tail = "tail";
        std::string marker = "DEVDOCK_ARGUMENT_TEST=synthetic_only";
        std::array<char*, 5> arguments{empty.data(), empty.data(), tail.data(), empty.data(), nullptr};
        std::array<char*, 2> environment{marker.data(), nullptr};
        ::execve(executable, arguments.data(), environment.data());
        std::cerr << "Unable to re-execute argument test with empty argv[0].\n";
        return EXIT_FAILURE;
    }
}

int main(int argc, char* argv[]) {
    try {
        if (argc == 2 && std::string_view{argv[1]} == "--empty-argv0") {
            return inspect_with_empty_argv0(argv[0]);
        }

        decoder_preserves_empty_arguments_and_excludes_environment();
        decoder_keeps_arguments_unformatted();
        decoder_respects_executable_path_alignment();
        decoder_rejects_malformed_input();

        const auto process = devdock::MacProcessInspector{}.inspect(::getpid());
        const std::vector<std::string> expected{argv, argv + argc};
        require(process.has_value(), "Unable to inspect current process.");
        if (!process->arguments) {
            throw std::runtime_error{"Current process arguments are unavailable."};
        }
        require(*process->arguments == expected, "Process inspection did not preserve exact arguments or included environment entries.");
        std::cout << "All process argument tests passed\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
