#pragma once

#include "domain/error.hpp"
#include "domain/process_identity.hpp"

namespace devdock::mac_detail {

    struct ProcessSnapshot {
        ProcessIdentity identity;
        bool zombie;
    };

    Result<ProcessSnapshot> read_process_snapshot(
        ProcessId pid
    );

    Result<ProcessIdentity> read_process_identity(
        ProcessId pid
    );

}