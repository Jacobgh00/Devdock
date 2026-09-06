#include "list_ports.hpp"

#include <algorithm>
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

    Result<std::vector<PortDetails>> ListPorts::execute() const {
        auto listeners = port_inspector_.listening_ports();

        if (!listeners) {
            return std::unexpected(listeners.error());
        }

        std::vector<PortDetails> result;
        result.reserve(listeners->size());

        for (const auto& listener : *listeners) {
            result.push_back(PortDetails{.listener = listener, .process = inspect_process(process_inspector_, listener.pid)});
        }

        std::ranges::sort(
            result,
            {},
            [](const PortDetails& details) {
                return details.listener.port;
            }
        );

        return result;
    }
}
