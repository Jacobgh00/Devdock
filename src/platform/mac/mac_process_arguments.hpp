#pragma once

#include "domain/error.hpp"

#include <span>
#include <string>
#include <vector>

namespace devdock::mac_detail {
    // Decodes KERN_PROCARGS2 data for 64-bit macOS processes, excluding environment strings.
    [[nodiscard]] Result<std::vector<std::string>> decode_process_arguments(std::span<const char> buffer);
}
