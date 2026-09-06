#include "platform/mac/mac_port_inspector.hpp"

#include <algorithm>
#include <arpa/inet.h>
#include <array>
#include <libproc.h>
#include <optional>
#include <sys/proc_info.h>
#include <tuple>
#include <utility>
#include <vector>

namespace devdock {

    namespace {

        std::vector<pid_t> list_process_ids() {
            int capacity =
                proc_listallpids(
                    nullptr,
                    0
                );

            if (capacity <= 0) {
                return {};
            }

            capacity += 64;

            for (
                int attempt = 0;
                attempt < 3;
                ++attempt) {
                std::vector<pid_t> pids(
                    static_cast<std::size_t>(
                        capacity
                    )
                );

                const int count =
                    proc_listallpids(
                        pids.data(),
                        static_cast<int>(
                            pids.size() * sizeof(pid_t)
                        )
                    );

                if (count <= 0) {
                    return {};
                }

                if (count < capacity) {
                    pids.resize(
                        static_cast<std::size_t>(
                            count
                        )
                    );

                    std::erase_if(
                        pids,
                        [](pid_t pid) {
                            return pid <= 0;
                        }
                    );

                    return pids;
                }

                capacity *= 2;
            }

            return {};
        }

        std::vector<proc_fdinfo>
        list_file_descriptors(
            pid_t pid
        ) {
            const int required_bytes =
                proc_pidinfo(
                    pid,
                    PROC_PIDLISTFDS,
                    0,
                    nullptr,
                    0
                );

            if (required_bytes <= 0) {
                return {};
            }

            const auto capacity =
                (static_cast<std::size_t>(
                     required_bytes
                 ) /
                 sizeof(proc_fdinfo)) +
                16;

            std::vector<proc_fdinfo>
                descriptors(capacity);

            const int bytes =
                proc_pidinfo(
                    pid,
                    PROC_PIDLISTFDS,
                    0,
                    descriptors.data(),
                    static_cast<int>(
                        descriptors.size() * sizeof(proc_fdinfo)
                    )
                );

            if (bytes <= 0) {
                return {};
            }

            descriptors.resize(
                static_cast<std::size_t>(
                    bytes
                ) /
                sizeof(proc_fdinfo)
            );

            return descriptors;
        }

        std::optional<socket_fdinfo>
        read_socket_info(
            pid_t pid,
            int descriptor
        ) {
            socket_fdinfo info{};

            const int bytes =
                proc_pidfdinfo(
                    pid,
                    descriptor,
                    PROC_PIDFDSOCKETINFO,
                    &info,
                    PROC_PIDFDSOCKETINFO_SIZE
                );

            if (
                bytes != PROC_PIDFDSOCKETINFO_SIZE
            ) {
                return std::nullopt;
            }

            return info;
        }

        std::optional<Protocol>
        socket_protocol(
            const in_sockinfo& info
        ) {
            if (
                (
                    info.insi_vflag & INI_IPV4
                ) != 0
            ) {
                return Protocol::tcp;
            }

            if (
                (
                    info.insi_vflag & INI_IPV6
                ) != 0
            ) {
                return Protocol::tcp6;
            }

            return std::nullopt;
        }

        std::optional<std::string>
        socket_address(
            const in_sockinfo& info,
            Protocol protocol
        ) {
            if (
                protocol == Protocol::tcp
            ) {
                std::array<
                    char,
                    INET_ADDRSTRLEN>
                    output{};

                if (
                    ::inet_ntop(
                        AF_INET,
                        &info
                             .insi_laddr
                             .ina_46
                             .i46a_addr4,
                        output.data(),
                        output.size()
                    ) == nullptr
                ) {
                    return std::nullopt;
                }

                return std::string{
                    output.data()
                };
            }

            std::array<
                char,
                INET6_ADDRSTRLEN>
                output{};

            if (
                ::inet_ntop(
                    AF_INET6,
                    &info
                         .insi_laddr
                         .ina_6,
                    output.data(),
                    output.size()
                ) == nullptr
            ) {
                return std::nullopt;
            }

            return std::string{
                output.data()
            };
        }

        std::optional<ListeningPort>
        listening_port_from_socket(
            pid_t pid,
            const socket_fdinfo& socket
        ) {
            if (
                socket.psi.soi_kind != SOCKINFO_TCP
            ) {
                return std::nullopt;
            }

            const auto& tcp =
                socket.psi
                    .soi_proto
                    .pri_tcp;

            if (
                tcp.tcpsi_state != TSI_S_LISTEN
            ) {
                return std::nullopt;
            }

            const auto& info =
                tcp.tcpsi_ini;

            const auto protocol =
                socket_protocol(info);

            if (!protocol) {
                return std::nullopt;
            }

            const auto address =
                socket_address(
                    info,
                    *protocol
                );

            if (!address) {
                return std::nullopt;
            }

            return ListeningPort{
                .port =
                    ntohs(
                        static_cast<
                            std::uint16_t>(info.insi_lport)
                    ),
                .protocol = *protocol,
                .address = *address,
                .pid = pid,
            };
        }

        void append_process_listeners(
            pid_t pid,
            std::vector<ListeningPort>&
                listeners
        ) {
            const auto descriptors =
                list_file_descriptors(pid);

            for (
                const auto& descriptor : descriptors) {
                if (
                    descriptor.proc_fdtype != PROX_FDTYPE_SOCKET
                ) {
                    continue;
                }

                const auto socket =
                    read_socket_info(
                        pid,
                        descriptor.proc_fd
                    );

                if (!socket) {
                    continue;
                }

                const auto listener =
                    listening_port_from_socket(
                        pid,
                        *socket
                    );

                if (listener) {
                    listeners.push_back(
                        *listener
                    );
                }
            }
        }

        void sort_and_deduplicate(
            std::vector<ListeningPort>&
                listeners
        ) {
            std::ranges::sort(
                listeners,
                {},
                [](const ListeningPort& listener) {
                    return std::tuple{
                        listener.port,
                        listener.pid,
                        listener.protocol,
                        listener.address
                    };
                }
            );

            const auto duplicates =
                std::ranges::unique(
                    listeners,
                    [](
                        const ListeningPort& left,
                        const ListeningPort& right
                    ) {
                        return left.port == right.port && left.pid == right.pid && left.protocol == right.protocol && left.address == right.address;
                    }
                );

            listeners.erase(
                duplicates.begin(),
                duplicates.end()
            );
        }

    } // namespace

    Result<std::vector<ListeningPort>>
    MacPortInspector::listening_ports() const {
        const auto pids =
            list_process_ids();

        if (pids.empty()) {
            return std::unexpected(
                Error{
                    .code = ErrorCode::system_error,
                    .message = "Unable to enumerate processes "
                               "with libproc.",
                }
            );
        }

        std::vector<ListeningPort>
            listeners;

        for (const auto pid : pids) {
            append_process_listeners(
                pid,
                listeners
            );
        }

        sort_and_deduplicate(
            listeners
        );

        return listeners;
    }

} // namespace devdock