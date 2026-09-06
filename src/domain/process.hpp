#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "process_identity.hpp"

namespace devdock {
    struct Process {
        ProcessIdentity identity;
        std::string name;
        std::optional<std::vector<std::string>> arguments;
        std::optional<std::filesystem::path> working_directory;
    };
}
