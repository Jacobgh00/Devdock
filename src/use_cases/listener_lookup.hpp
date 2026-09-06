#pragma once

#include "domain/error.hpp"
#include "domain/listening_port.hpp"
#include "domain/port_scan.hpp"
#include "domain/process_identity.hpp"

#include <cstdint>
#include <vector>

namespace devdock {

    // Answers what one scan says about one port. Absence is qualified by the
    // scan's own visibility: a port with no listener is reported as unseen rather
    // than as empty whenever part of the scan could not be inspected.

    [[nodiscard]] Result<std::vector<ListeningPort>> listeners_on(
        const PortScan& scan,
        std::uint16_t port
    );

    // Resolves the port to the single process that owns it. Two processes can
    // listen on one port on different addresses or address families, which is
    // ambiguous rather than a choice this program may make.
    [[nodiscard]] Result<ProcessId> unique_owner_of(
        const PortScan& scan,
        std::uint16_t port
    );

}
