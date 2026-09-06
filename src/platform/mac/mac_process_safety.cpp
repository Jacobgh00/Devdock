#include "platform/mac/mac_process_safety.hpp"

#include <unistd.h>

namespace devdock {

    Result<void> validate_process_target(
        const ProcessIdentity& identity,
        const ProcessSafetyContext& context
    ) {
        if (context.effective_user_id == 0) {
            return std::unexpected(
                Error{
                    .code = ErrorCode::permission_denied,
                    .message = "DevDock refuses destructive operations "
                               "while running as root.",
                }
            );
        }

        if (identity.pid <= 1) {
            return std::unexpected(
                Error{
                    .code = ErrorCode::protected_process,
                    .message = "DevDock refuses to terminate "
                               "protected system processes.",
                }
            );
        }

        if (identity.pid == context.self_pid) {
            return std::unexpected(
                Error{
                    .code = ErrorCode::protected_process,
                    .message = "DevDock refuses to terminate itself.",
                }
            );
        }

        if (
            identity.owner_user_id != context.effective_user_id
        ) {
            return std::unexpected(
                Error{
                    .code = ErrorCode::permission_denied,
                    .message = "DevDock only terminates processes "
                               "owned by the current user.",
                }
            );
        }

        return {};
    }

    Result<void> validate_process_target(
        const ProcessIdentity& identity
    ) {
        return validate_process_target(
            identity,
            ProcessSafetyContext{
                .self_pid = getpid(),
                .effective_user_id = geteuid(),
            }
        );
    }

}