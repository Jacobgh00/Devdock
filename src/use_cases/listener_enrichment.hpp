#pragma once

#include "domain/listening_port.hpp"
#include "ports/process_inspector.hpp"
#include "use_cases/port_details.hpp"

#include <span>
#include <vector>

namespace devdock {
    // Produces details for the given listener rows, inspecting each distinct PID
    // once per call. Listeners whose process cannot be inspected are kept without
    // metadata. Reuse is scoped to one call and must never span a destructive
    // revalidation.
    [[nodiscard]] std::vector<PortDetails> enrich_listeners(
        const ProcessInspector& process_inspector,
        std::span<const ListeningPort> listeners
    );
}
