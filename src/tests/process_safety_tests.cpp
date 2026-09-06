#include "tests/test_support.hpp"

#include "platform/mac/mac_process_safety.hpp"

#include <cstdlib>
#include <exception>
#include <iostream>

namespace {

    using namespace devdock;
    using namespace devdock::test_support;

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
    return run_suite(
        "process safety",
        {
            refuses_root,
            refuses_pid_one,
            refuses_self,
            refuses_other_user,
            accepts_current_users_process,
        }
    );
}
