#include "platform/mac/mac_process_inspector.hpp"

#include "platform/mac/mac_process_identity.hpp"

#include <array>
#include <cstring>
#include <filesystem>
#include <libproc.h>
#include <optional>
#include <string>
#include <string_view>
#include <sys/proc_info.h>
#include <sys/sysctl.h>
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

        std::string quote_argument(
            std::string_view argument
        ) {
            if (
                argument.find_first_of(
                    " \t\"'"
                ) == std::string_view::npos
            ) {
                return std::string{
                    argument
                };
            }

            std::string result{"'"};

            for (const char character : argument) {
                if (character == '\'') {
                    result += "'\\''";
                } else {
                    result += character;
                }
            }

            result += '\'';

            return result;
        }

        std::optional<std::vector<char>>
        read_process_arguments(
            ProcessId pid
        ) {
            int argument_limit = 0;

            size_t limit_size =
                sizeof(argument_limit);

            int limit_mib[2] = {
                CTL_KERN,
                KERN_ARGMAX,
            };

            if (
                ::sysctl(
                    limit_mib,
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

            int arguments_mib[3] = {
                CTL_KERN,
                KERN_PROCARGS2,
                pid,
            };

            if (
                ::sysctl(
                    arguments_mib,
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

        std::optional<std::string>
        command_from_arguments(
            const std::vector<char>& buffer
        ) {
            if (
                buffer.size() <= sizeof(int)
            ) {
                return std::nullopt;
            }

            int argc = 0;

            std::memcpy(
                &argc,
                buffer.data(),
                sizeof(argc)
            );

            if (argc <= 0) {
                return std::nullopt;
            }

            const char* cursor =
                buffer.data() + sizeof(argc);

            const char* end =
                buffer.data() + buffer.size();

            /*
             * KERN_PROCARGS2 places the executable path
             * before argv[0].
             */
            while (
                cursor < end && *cursor != '\0') {
                ++cursor;
            }

            /*
             * Skip null padding before argv[0].
             */
            while (
                cursor < end && *cursor == '\0') {
                ++cursor;
            }

            std::string command;

            for (
                int index = 0;
                index < argc && cursor < end;
                ++index) {
                const char* argument_end =
                    cursor;

                while (
                    argument_end < end && *argument_end != '\0') {
                    ++argument_end;
                }

                if (argument_end == end) {
                    break;
                }

                if (!command.empty()) {
                    command += ' ';
                }

                command += quote_argument(
                    std::string_view{
                        cursor,
                        static_cast<std::size_t>(
                            argument_end - cursor
                        ),
                    }
                );

                cursor =
                    argument_end + 1;

                while (
                    cursor < end && *cursor == '\0') {
                    ++cursor;
                }
            }

            if (command.empty()) {
                return std::nullopt;
            }

            return command;
        }

        std::optional<std::string>
        read_command(
            ProcessId pid
        ) {
            const auto arguments =
                read_process_arguments(pid);

            if (!arguments) {
                return std::nullopt;
            }

            return command_from_arguments(
                *arguments
            );
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

        const auto command =
            read_command(pid);

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
            .command =
                command.value_or(
                    "<unavailable>"
                ),
            .working_directory =
                working_directory,
        };
    }

} // namespace devdock