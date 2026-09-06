#pragma once

#include "ports/process_controller.hpp"

namespace devdock {

    class MacProcessController final
        : public ProcessController {
    public:
        Result<bool> stop(
            const ProcessIdentity& identity,
            TerminationMode mode,
            std::chrono::milliseconds timeout
        ) const override;
    };

}