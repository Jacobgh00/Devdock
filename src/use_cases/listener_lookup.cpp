#include "use_cases/listener_lookup.hpp"

#include <set>
#include <string>

namespace devdock {

    namespace {

        Error no_listener_error(
            const PortScan& scan,
            std::uint16_t port
        ) {
            if (scan.uninspectable_processes == 0) {
                return Error{
                    .code = ErrorCode::not_found,
                    .message = "No process is listening on port " + std::to_string(port) + ".",
                };
            }

            return Error{
                .code = ErrorCode::not_found,
                .message = "No visible process is listening on port " + std::to_string(port) + ". " + std::to_string(scan.uninspectable_processes) + " of " + std::to_string(scan.scanned_processes) + " processes could not be inspected by this user.",
            };
        }

    } // namespace

    Result<std::vector<ListeningPort>> listeners_on(
        const PortScan& scan,
        std::uint16_t port
    ) {
        std::vector<ListeningPort> matches;

        for (const auto& listener : scan.listeners) {
            if (listener.port == port) {
                matches.push_back(listener);
            }
        }

        if (matches.empty()) {
            return std::unexpected(
                no_listener_error(scan, port)
            );
        }

        return matches;
    }

    Result<ProcessId> unique_owner_of(
        const PortScan& scan,
        std::uint16_t port
    ) {
        const auto matches = listeners_on(scan, port);

        if (!matches) {
            return std::unexpected(matches.error());
        }

        std::set<ProcessId> owners;

        for (const auto& listener : *matches) {
            owners.insert(listener.pid);
        }

        if (owners.size() > 1) {
            return std::unexpected(
                Error{
                    .code = ErrorCode::ambiguous_target,
                    .message = "Multiple processes are listening on port " + std::to_string(port) + "; use `devdock kill --pid <pid>` instead.",
                }
            );
        }

        return *owners.begin();
    }

}
