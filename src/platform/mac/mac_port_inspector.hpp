#pragma once

#include "ports/port_inspector.hpp"

namespace devdock {

    class MacPortInspector final : public PortInspector {
    public:
        [[nodiscard]] Result<PortScan>
        listening_ports() const override;
    };

}
