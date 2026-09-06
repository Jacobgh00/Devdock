#include "platform/mac/mac_process_arguments.hpp"

#include <cstring>
#include <string_view>

namespace devdock::mac_detail {
    namespace {
        std::unexpected<Error> malformed_arguments() {
            return std::unexpected(Error{
                .code = ErrorCode::system_error,
                .message = "Malformed KERN_PROCARGS2 process arguments.",
            });
        }
    }

    Result<std::vector<std::string>> decode_process_arguments(std::span<const char> buffer) {
        if (buffer.size() < sizeof(int)) {
            return malformed_arguments();
        }

        int argument_count = 0;
        std::memcpy(&argument_count, buffer.data(), sizeof(argument_count));
        if (argument_count <= 0) {
            return malformed_arguments();
        }

        const auto payload = buffer.subspan(sizeof(argument_count));
        const std::string_view strings{payload.data(), payload.size()};
        const auto path_end = strings.find('\0');

        if (path_end == std::string_view::npos) {
            return malformed_arguments();
        }

        // XNU exec_extract_strings aligns the saved path to the target pointer width.
        // Supported macOS processes are 64-bit. KERN_PROCARGS2 strips the 16-byte
        // "executable_path=" prefix, so this alignment is unchanged in the payload.
        // https://github.com/apple-oss-distributions/xnu/blob/main/bsd/kern/kern_exec.c
        constexpr std::size_t path_alignment = 8;
        const auto path_size = path_end + 1;
        const auto padding_size = (path_alignment - (path_size % path_alignment)) % path_alignment;

        if (padding_size > strings.size() - path_size ||
            strings.substr(path_size, padding_size).find_first_not_of('\0') != std::string_view::npos) {
            return malformed_arguments();
        }

        auto remaining = strings.substr(path_size + padding_size);

        if (static_cast<std::size_t>(argument_count) > remaining.size()) {
            return malformed_arguments();
        }

        std::vector<std::string> arguments;
        arguments.reserve(static_cast<std::size_t>(argument_count));

        for (int index = 0; index < argument_count; ++index) {
            const auto argument_end = remaining.find('\0');

            if (argument_end == std::string_view::npos) {
                return malformed_arguments();
            }

            arguments.emplace_back(remaining.substr(0, argument_end));
            remaining.remove_prefix(argument_end + 1);
        }
        return arguments;
    }
}
