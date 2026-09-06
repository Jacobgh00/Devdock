#pragma once

#include "domain/error.hpp"
#include "domain/process.hpp"

namespace devdock {
    class ProcessInspector {
    public:
        virtual ~ProcessInspector() = default;

    protected:
        ProcessInspector() = default;
        ProcessInspector(const ProcessInspector&) = default;
        ProcessInspector(ProcessInspector&&) = default;
        ProcessInspector& operator=(const ProcessInspector&) = default;
        ProcessInspector& operator=(ProcessInspector&&) = default;

    public:
        [[nodiscard]] virtual Result<Process> inspect(
            ProcessId pid
        ) const = 0;
    };
}
