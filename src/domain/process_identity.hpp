#pragma once

#include <cstdint>

namespace devdock {
    using ProcessId = int;
    using UserId = std::uint32_t;

    struct ProcessIdentity {
        ProcessId pid;
        UserId owner_user_id;
        std::uint64_t start_time_token;

        friend bool operator==(
            const ProcessIdentity&,
            const ProcessIdentity&
        ) = default;
    };
}