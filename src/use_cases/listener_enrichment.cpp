#include "use_cases/listener_enrichment.hpp"

#include <optional>
#include <unordered_map>
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

    } // namespace

    std::vector<PortDetails> enrich_listeners(
        const ProcessInspector& process_inspector,
        std::span<const ListeningPort> listeners
    ) {
        std::unordered_map<ProcessId, std::optional<Process>> resolved;

        std::vector<PortDetails> details;
        details.reserve(listeners.size());

        for (const auto& listener : listeners) {
            const auto [entry, first_sighting] =
                resolved.try_emplace(listener.pid);

            if (first_sighting) {
                entry->second =
                    inspect_process(
                        process_inspector,
                        listener.pid
                    );
            }

            details.push_back(PortDetails{
                .listener = listener,
                .process = entry->second,
            });
        }

        return details;
    }

}
