#pragma once

#include "ports/process_inspector.hpp"

namespace devdock {
    class MacProcessInspector final : public ProcessInspector {
    public:
        Result<Process> inspect(
            ProcessId pid
        ) const override;
    };

}