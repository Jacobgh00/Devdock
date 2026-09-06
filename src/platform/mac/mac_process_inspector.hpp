#pragma once

#include "ports/process_inspector.hpp"

namespace devdock {
    class MacProcessInspector final : public ProcessInspector {
    public:
        [[nodiscard]] Result<Process> inspect(
            ProcessId pid
        ) const override;
    };

}