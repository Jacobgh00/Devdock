#pragma once

#include "domain/error.hpp"
#include "domain/port_scan.hpp"

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
        [[nodiscard]] virtual Result<PortScan> listening_ports() const = 0;
    };
}
