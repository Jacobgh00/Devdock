#include "use_cases/inspect_port.hpp"

#include <optional>
#include <utility>

namespace devdock {

    namespace {
        std::optional<Process> inspect_process(
            const ProcessInspector& inspector,
            ProcessId pid
        ) {
            auto process = inspector.inspect(pid);

            if (!process) {
                return std::nullopt;
            }

            return std::move(*process);
        }
    }

    Result<std::vector<PortDetails>> InspectPort::execute(std::uint16_t port) const {
        auto listeners =
            port_inspector_.listening_ports();

        if (!listeners) {
            return std::unexpected(
                listeners.error()
            );
        }

        std::vector<PortDetails> matches;

        for (const auto& listener : *listeners) {
            if (listener.port != port) {
                continue;
            }

            matches.push_back(PortDetails{
                .listener = listener,
                .process = inspect_process(process_inspector_, listener.pid),
            });
        }

        if (matches.empty()) {
            return std::unexpected(Error{
                ErrorCode::not_found,
                "No process is listening on port " + std::to_string(port) + ".",
            });
        }

        return matches;
    }

}
