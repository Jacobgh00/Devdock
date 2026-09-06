#include "platform/mac/mac_process_safety.hpp"

#include <cstdlib>
#include <exception>
#include <iostream>

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

    ProcessIdentity target(
        ProcessId pid,
        UserId owner
    ) {
        return ProcessIdentity{
            .pid = pid,
            .owner_user_id = owner,
            .start_time_token = 100,
        };
    }

    void refuses_root() {
        const auto result =
            validate_process_target(
                target(
                    42,
                    0
                ),
                ProcessSafetyContext{
                    .self_pid = 100,
                    .effective_user_id = 0,
                }
            );

        CHECK(!result.has_value());

        CHECK(
            result.error().code == ErrorCode::permission_denied
        );
    }

    void refuses_pid_one() {
        const auto result =
            validate_process_target(
                target(
                    1,
                    501
                ),
                ProcessSafetyContext{
                    .self_pid = 100,
                    .effective_user_id = 501,
                }
            );

        CHECK(!result.has_value());

        CHECK(
            result.error().code == ErrorCode::protected_process
        );
    }

    void refuses_self() {
        const auto result =
            validate_process_target(
                target(
                    100,
                    501
                ),
                ProcessSafetyContext{
                    .self_pid = 100,
                    .effective_user_id = 501,
                }
            );

        CHECK(!result.has_value());

        CHECK(
            result.error().code == ErrorCode::protected_process
        );
    }

    void refuses_other_user() {
        const auto result =
            validate_process_target(
                target(
                    42,
                    502
                ),
                ProcessSafetyContext{
                    .self_pid = 100,
                    .effective_user_id = 501,
                }
            );

        CHECK(!result.has_value());

        CHECK(
            result.error().code == ErrorCode::permission_denied
        );
    }

    void accepts_current_users_process() {
        const auto result =
            validate_process_target(
                target(
                    42,
                    501
                ),
                ProcessSafetyContext{
                    .self_pid = 100,
                    .effective_user_id = 501,
                }
            );

        CHECK(result.has_value());
    }

} // namespace

int main() {
    try {
        refuses_root();
        refuses_pid_one();
        refuses_self();
        refuses_other_user();
        accepts_current_users_process();

        if (failure_count() != 0) {
            std::cerr
                << failure_count()
                << " test(s) failed\n";

            return EXIT_FAILURE;
        }

        std::cout
            << "All process safety tests passed\n";

        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr
            << "Unexpected exception: "
            << error.what()
            << '\n';

        return 1;
    }
}
