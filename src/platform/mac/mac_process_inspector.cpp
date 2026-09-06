#include "platform/mac/mac_process_inspector.hpp"

#include "platform/mac/mac_process_arguments.hpp"
#include "platform/mac/mac_process_identity.hpp"

#include <array>
#include <filesystem>
#include <libproc.h>
#include <optional>
#include <string>
#include <sys/proc_info.h>
#include <sys/sysctl.h>
#include <utility>
#include <vector>

namespace devdock {

    namespace {

        std::optional<std::string>
        read_process_name(
            ProcessId pid
        ) {
            std::array<
                char,
                PROC_PIDPATHINFO_MAXSIZE>
                buffer{};

            if (
                proc_name(
                    pid,
                    buffer.data(),
                    static_cast<std::uint32_t>(
                        buffer.size()
                    )
                ) > 0
            ) {
                return std::string{
                    buffer.data()
                };
            }

            if (
                proc_pidpath(
                    pid,
                    buffer.data(),
                    static_cast<std::uint32_t>(
                        buffer.size()
                    )
                ) > 0
            ) {
                return std::filesystem::path{
                    buffer.data()
                }
                    .filename()
                    .string();
            }

            return std::nullopt;
        }

        std::optional<std::vector<char>>
        read_argument_buffer(
            ProcessId pid
        ) {
            int argument_limit = 0;

            size_t limit_size =
                sizeof(argument_limit);

            std::array<int, 2> limit_mib = {
                CTL_KERN,
                KERN_ARGMAX,
            };

            if (
                ::sysctl(
                    limit_mib.data(),
                    2,
                    &argument_limit,
                    &limit_size,
                    nullptr,
                    0
                ) != 0 ||
                argument_limit <= 0
            ) {
                return std::nullopt;
            }

            std::vector<char> buffer(
                static_cast<std::size_t>(
                    argument_limit
                )
            );

            size_t size =
                buffer.size();

            std::array<int, 3> arguments_mib = {
                CTL_KERN,
                KERN_PROCARGS2,
                pid,
            };

            if (
                ::sysctl(
                    arguments_mib.data(),
                    3,
                    buffer.data(),
                    &size,
                    nullptr,
                    0
                ) != 0 ||
                size <= sizeof(int)
            ) {
                return std::nullopt;
            }

            buffer.resize(size);

            return buffer;
        }

        std::optional<std::vector<std::string>>
        read_process_arguments(
            ProcessId pid
        ) {
            const auto buffer = read_argument_buffer(pid);

            if (!buffer) {
                return std::nullopt;
            }

            auto arguments = mac_detail::decode_process_arguments(*buffer);
            if (!arguments) {
                return std::nullopt;
            }
            return std::move(*arguments);
        }

        std::optional<std::filesystem::path>
        read_working_directory(
            ProcessId pid
        ) {
            proc_vnodepathinfo info{};

            const int bytes =
                proc_pidinfo(
                    pid,
                    PROC_PIDVNODEPATHINFO,
                    0,
                    &info,
                    PROC_PIDVNODEPATHINFO_SIZE
                );

            if (
                bytes != PROC_PIDVNODEPATHINFO_SIZE || info.pvi_cdir
                                                               .vip_path[0] == '\0'
            ) {
                return std::nullopt;
            }

            return std::filesystem::path{
                info.pvi_cdir.vip_path
            };
        }

        Result<void> confirm_identity(
            const ProcessIdentity& expected
        ) {
            auto current =
                mac_detail::read_process_identity(
                    expected.pid
                );

            if (!current) {
                return std::unexpected(
                    current.error()
                );
            }

            if (*current != expected) {
                return std::unexpected(
                    Error{
                        .code = ErrorCode::target_changed,
                        .message = "Process changed while its metadata "
                                   "was being inspected.",
                    }
                );
            }

            return {};
        }

    } // namespace

    Result<Process>
    MacProcessInspector::inspect(
        ProcessId pid
    ) const {
        auto identity =
            mac_detail::read_process_identity(
                pid
            );

        if (!identity) {
            return std::unexpected(
                identity.error()
            );
        }

        auto name =
            read_process_name(pid);

        if (!name) {
            return std::unexpected(
                Error{
                    .code = ErrorCode::system_error,
                    .message = "Unable to read process name for PID " + std::to_string(pid) + ".",
                }
            );
        }

        auto arguments = read_process_arguments(pid);

        const auto working_directory =
            read_working_directory(pid);

        auto confirmed =
            confirm_identity(
                *identity
            );

        if (!confirmed) {
            return std::unexpected(
                confirmed.error()
            );
        }

        return Process{
            .identity = *identity,
            .name = std::move(*name),
            .arguments = std::move(arguments),
            .working_directory =
                working_directory,
        };
    }

} // namespace devdock
