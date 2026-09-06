#include "platform/mac/mac_process_identity.hpp"

#include <cerrno>
#include <cstring>
#include <libproc.h>
#include <sys/proc.h>
#include <sys/proc_info.h>

namespace devdock::mac_detail {

    namespace {

        Error process_identity_error(
            ProcessId pid
        ) {
            const auto code =
                errno == ESRCH
                    ? ErrorCode::not_found
                : (
                      errno == EPERM || errno == EACCES
                  )
                    ? ErrorCode::permission_denied
                    : ErrorCode::system_error;

            return Error{
                .code = code,
                .message = "Unable to read process identity for PID " + std::to_string(pid) + ": " + std::strerror(errno),
            };
        }

    } // namespace

    Result<ProcessSnapshot>
    read_process_snapshot(
        ProcessId pid
    ) {
        if (pid <= 0) {
            return std::unexpected(
                Error{
                    .code = ErrorCode::invalid_argument,
                    .message = "PID must be greater than zero.",
                }
            );
        }

        proc_bsdinfo info{};

        errno = 0;

        const int bytes =
            proc_pidinfo(
                pid,
                PROC_PIDTBSDINFO,
                0,
                &info,
                PROC_PIDTBSDINFO_SIZE
            );

        if (
            bytes != PROC_PIDTBSDINFO_SIZE
        ) {
            return std::unexpected(
                process_identity_error(pid)
            );
        }

        constexpr std::uint64_t
            microseconds_per_second =
                1'000'000;

        return ProcessSnapshot{
            .identity =
                ProcessIdentity{
                    .pid = pid,
                    .owner_user_id =
                        static_cast<UserId>(
                            info.pbi_uid
                        ),
                    .start_time_token =
                        info.pbi_start_tvsec * microseconds_per_second + info.pbi_start_tvusec,
                },
            .zombie =
                info.pbi_status == SZOMB,
        };
    }

    Result<ProcessIdentity>
    read_process_identity(
        ProcessId pid
    ) {
        auto snapshot =
            read_process_snapshot(pid);

        if (!snapshot) {
            return std::unexpected(
                snapshot.error()
            );
        }

        return snapshot->identity;
    }

}