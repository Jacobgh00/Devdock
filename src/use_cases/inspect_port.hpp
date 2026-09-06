#pragma once

#include "domain/error.hpp"
#include "ports/port_inspector.hpp"
#include "ports/process_inspector.hpp"
#include "use_cases/port_details.hpp"

#include <cstdint>
#include <vector>

namespace devdock {
    class InspectPort {
    public:
        InspectPort(
            const PortInspector& port_inspector,
            const ProcessInspector& process_inspector
        ) : port_inspector_(port_inspector),
            process_inspector_(process_inspector) {}

        Result<std::vector<PortDetails>>
        execute(std::uint16_t port) const;

    private:
        const PortInspector& port_inspector_;
        const ProcessInspector& process_inspector_;
    };
}