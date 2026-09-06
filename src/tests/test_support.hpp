#pragma once

#include <cstdlib>
#include <exception>
#include <initializer_list>
#include <iostream>
#include <string_view>

// Assertion semantics and suite reporting shared by the hand-written test
// suites. Deliberately not a testing framework: it offers exactly two
// assertions and one runner.
namespace devdock::test_support {

    inline int& failure_count() {
        static int failures = 0;

        return failures;
    }

    inline void record_failure(
        std::string_view file,
        int line,
        std::string_view expression
    ) {
        std::cerr
            << file
            << ':'
            << line
            << " failed: "
            << expression
            << '\n';

        ++failure_count();
    }

    inline int run_suite(
        std::string_view name,
        std::initializer_list<void (*)()> tests
    ) {
        try {
            for (const auto test : tests) {
                test();
            }

            if (failure_count() != 0) {
                std::cerr
                    << failure_count()
                    << " test(s) failed\n";

                return EXIT_FAILURE;
            }

            std::cout
                << "All "
                << name
                << " tests passed\n";

            return EXIT_SUCCESS;
        } catch (const std::exception& error) {
            std::cerr
                << "Unexpected exception: "
                << error.what()
                << '\n';

            return EXIT_FAILURE;
        }
    }

}

// Records a failure and lets the test continue. Use for comparisons.
#define CHECK(condition)                           \
    do {                                           \
        if (!(condition)) {                        \
            devdock::test_support::record_failure( \
                __FILE__,                          \
                __LINE__,                          \
                "CHECK " #condition                \
            );                                     \
        }                                          \
    } while (false)

// Records a failure and abandons the test. Use for preconditions whose failure
// would make the rest of the test meaningless or unsafe to run.
#define REQUIRE(condition)                         \
    do {                                           \
        if (!(condition)) {                        \
            devdock::test_support::record_failure( \
                __FILE__,                          \
                __LINE__,                          \
                "REQUIRE " #condition              \
            );                                     \
                                                   \
            return;                                \
        }                                          \
    } while (false)
