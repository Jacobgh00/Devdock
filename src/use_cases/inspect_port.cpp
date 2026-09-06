#include "use_cases/inspect_port.hpp"

#include "use_cases/listener_enrichment.hpp"
#include "use_cases/listener_lookup.hpp"

namespace devdock {

    Result<std::vector<PortDetails>> InspectPort::execute(std::uint16_t port) const {
        const auto scan =
            port_inspector_.listening_ports();

        if (!scan) {
            return std::unexpected(
                scan.error()
            );
        }

        const auto matches =
            listeners_on(*scan, port);

        if (!matches) {
            return std::unexpected(
                matches.error()
            );
        }

        return enrich_listeners(
            process_inspector_,
            *matches
        );
    }

}
