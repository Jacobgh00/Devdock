#pragma once

#include "domain/error.hpp"
#include "domain/listening_port.hpp"

#include <vector>

namespace devdock {
    class PortInspector {
    public:
        virtual ~PortInspector() = default;

    protected:
        PortInspector() = default;
        PortInspector(const PortInspector&) = default;
        PortInspector(PortInspector&&) = default;
        PortInspector& operator=(const PortInspector&) = default;
        PortInspector& operator=(PortInspector&&) = default;

    public:
        [[nodiscard]] virtual Result<std::vector<ListeningPort>> listening_ports() const = 0;
    };
}
