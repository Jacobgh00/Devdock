#pragma once

#include <cstdint>

namespace devdock {
    enum class Protocol : std::uint8_t {
        tcp,
        tcp6,
    };
}