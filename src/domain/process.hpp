#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include "process_identity.hpp"

namespace devdock {
    struct Process {
        ProcessIdentity identity;
        std::string name;
        std::string command;
        std::optional<std::filesystem::path> working_directory;
    };
}
