#pragma once

#include "ports/port_inspector.hpp"

namespace devdock {

    class MacPortInspector final : public PortInspector {
    public:
        Result<std::vector<ListeningPort>>
        listening_ports() const override;
    };

}