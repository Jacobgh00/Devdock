#pragma once

#include "domain/error.hpp"
#include "domain/process.hpp"

namespace devdock {
    class ProcessInspector {
    public:
        virtual ~ProcessInspector() = default;

        [[nodiscard]] virtual Result<Process> inspect(
            ProcessId pid
        ) const = 0;
    };
}
