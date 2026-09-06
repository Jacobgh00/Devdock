#pragma once

#include "ports/process_controller.hpp"

namespace devdock {

    class MacProcessController final
        : public ProcessController {
    public:
        [[nodiscard]] Result<StopOutcome> stop(
            const ProcessIdentity& identity,
            TerminationMode mode,
            std::chrono::milliseconds timeout
        ) const override;
    };

}