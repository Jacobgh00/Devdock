#include "cli/cli.hpp"
#include "platform/mac/mac_port_inspector.hpp"
#include "platform/mac/mac_process_controller.hpp"
#include "platform/mac/mac_process_inspector.hpp"

#include <span>

int main(
    int argc,
    char* argv[]
) {
    devdock::MacPortInspector
        port_inspector;

    devdock::MacProcessInspector
        process_inspector;

    devdock::MacProcessController
        process_controller;

    return devdock::run_cli(
        std::span<char* const>{
            argv,
            static_cast<std::size_t>(argc)
        },
        port_inspector,
        process_inspector,
        process_controller
    );
}