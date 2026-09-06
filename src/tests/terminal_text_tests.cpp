#include "cli/terminal_text.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

    using devdock::sanitize_terminal_text;

    int failures = 0;

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
            ++failures;              \
        }                            \
    } while (false)

    void keeps_normal_utf8() {
        CHECK(
            sanitize_terminal_text(
                "Jyllands-Posten æøå"
            ) ==
            "Jyllands-Posten æøå"
        );
    }

    void escapes_ascii_controls() {
        CHECK(
            sanitize_terminal_text(
                "hello\nworld\t!"
            ) ==
            "hello\\nworld\\t!"
        );
    }

    void escapes_ansi_escape_sequences() {
        const std::string input =
            "safe\x1B[2Junsafe";

        CHECK(
            sanitize_terminal_text(
                input
            ) ==
            "safe\\x1B[2Junsafe"
        );
    }

    void escapes_unicode_c1_controls() {
        const std::string input =
            "safe\xC2\x9B"
            "31m";

        CHECK(
            sanitize_terminal_text(
                input
            ) ==
            "safe\\u009B31m"
        );
    }

    void escapes_invalid_utf8() {
        const std::string input{
            "safe\x9Bunsafe",
            11
        };

        CHECK(
            sanitize_terminal_text(
                input
            ) ==
            "safe\\x9Bunsafe"
        );
    }

} // namespace

int main() {
    keeps_normal_utf8();
    escapes_ascii_controls();
    escapes_ansi_escape_sequences();
    escapes_unicode_c1_controls();
    escapes_invalid_utf8();

    if (failures != 0) {
        std::cerr
            << failures
            << " test(s) failed\n";

        return EXIT_FAILURE;
    }

    std::cout
        << "All terminal text tests passed\n";

    return EXIT_SUCCESS;
}