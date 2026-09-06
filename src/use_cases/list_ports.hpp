#pragma once

#include "domain/error.hpp"
#include "port_details.hpp"
#include "ports/port_inspector.hpp"
#include "ports/process_inspector.hpp"

#include <vector>

namespace devdock {
    class ListPorts {
    public:
        ListPorts(
            const PortInspector& port_inspector,
            const ProcessInspector& process_inspector
        )
            : port_inspector_{port_inspector},
              process_inspector_{process_inspector} {}

        Result<std::vector<PortDetails>>
        execute() const;

    private:
        const PortInspector& port_inspector_;
        const ProcessInspector& process_inspector_;
    };
}