#pragma once

#include "domain/error.hpp"
#include "domain/process_identity.hpp"

namespace devdock {

    struct ProcessSafetyContext {
        ProcessId self_pid;
        UserId effective_user_id;
    };

    Result<void> validate_process_target(
        const ProcessIdentity& identity,
        const ProcessSafetyContext& context
    );

    Result<void> validate_process_target(
        const ProcessIdentity& identity
    );

} // namespace devdock