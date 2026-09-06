#include "list_ports.hpp"

#include "use_cases/listener_enrichment.hpp"

#include <algorithm>

namespace devdock {

    Result<std::vector<PortDetails>> ListPorts::execute() const {
        auto scan = port_inspector_.listening_ports();

        if (!scan) {
            return std::unexpected(scan.error());
        }

        auto result =
            enrich_listeners(
                process_inspector_,
                scan->listeners
            );

        // Stable: the adapter already orders listeners by port, PID, protocol and
        // address, and that order is what rows of the same port keep.
        std::ranges::stable_sort(
            result,
            {},
            [](const PortDetails& details) {
                return details.listener.port;
            }
        );

        return result;
    }
}
