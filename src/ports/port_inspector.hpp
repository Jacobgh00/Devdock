#pragma once

#include "domain/error.hpp"
#include "domain/listening_port.hpp"

#include <vector>

namespace devdock {
    class PortInspector {
    public:
        virtual ~PortInspector() = default;

        [[nodiscard]] virtual Result<std::vector<ListeningPort>> listening_ports() const = 0;
    };
}
