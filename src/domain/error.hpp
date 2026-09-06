#pragma once

#include <cstdint>
#include <expected>
#include <string>

namespace devdock {
    enum class ErrorCode : std::uint8_t {
        invalid_argument,
        not_found,
        permission_denied,
        ambiguous_target,
        protected_process,
        target_changed,
        unsupported,
        system_error,
        termination_failed,
    };

    struct Error {
        ErrorCode code;
        std::string message;
    };

    template <typename T>
    using Result = std::expected<T, Error>;
}