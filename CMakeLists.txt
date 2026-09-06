cmake_minimum_required(VERSION 3.25)

project(devdock VERSION 0.3.0 LANGUAGES CXX)

if(NOT APPLE)
    message(FATAL_ERROR "DevDock currently supports macOS only")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(DEVDOCK_ENABLE_CLANG_TIDY "Run clang-tidy as part of the build" OFF)

if(DEVDOCK_ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_COMMAND NAMES clang-tidy REQUIRED)
    set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
endif()

function(devdock_enable_warnings target)
    target_compile_options(
            ${target}
            PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )
endfunction()

find_library(PROC_LIBRARY proc REQUIRED)

#
# Application core
#

add_library(
        devdock_core

        src/use_cases/inspect_port.cpp
        src/use_cases/kill_process.cpp
        src/use_cases/listener_enrichment.cpp
        src/use_cases/listener_lookup.cpp
        src/use_cases/list_ports.cpp
)

target_include_directories(
        devdock_core
        PUBLIC
        src
)

devdock_enable_warnings(devdock_core)

#
# CLI
#

add_library(
        devdock_cli

        src/cli/arguments.cpp
        src/cli/cli.cpp
        src/cli/formatter.cpp
        src/cli/terminal_text.cpp
)

target_include_directories(
        devdock_cli
        PUBLIC
        src
)

target_link_libraries(
        devdock_cli
        PUBLIC
        devdock_core
)

devdock_enable_warnings(devdock_cli)

#
# macOS platform adapter
#

add_library(
        devdock_platform

        src/platform/mac/mac_port_inspector.cpp
        src/platform/mac/mac_process_arguments.cpp
        src/platform/mac/mac_process_controller.cpp
        src/platform/mac/mac_process_identity.cpp
        src/platform/mac/mac_process_inspector.cpp
        src/platform/mac/mac_process_safety.cpp
)

target_include_directories(
        devdock_platform
        PUBLIC
        src
)

target_link_libraries(
        devdock_platform
        PUBLIC
        ${PROC_LIBRARY}
)

devdock_enable_warnings(devdock_platform)

#
# Executable
#

add_executable(
        devdock

        src/main.cpp
)

target_link_libraries(
        devdock
        PRIVATE
        devdock_cli
        devdock_platform
)

devdock_enable_warnings(devdock)

#
# Install
#

include(GNUInstallDirs)

install(
        TARGETS devdock
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

#
# Tests
#

include(CTest)

if(BUILD_TESTING)
    add_executable(
            devdock_application_tests

            src/tests/application_tests.cpp
    )

    target_link_libraries(
            devdock_application_tests
            PRIVATE
            devdock_core
    )

    devdock_enable_warnings(
            devdock_application_tests
    )

    add_test(
            NAME application
            COMMAND devdock_application_tests
    )

    add_executable(
            devdock_cli_tests

            src/tests/cli_tests.cpp
    )

    target_link_libraries(
            devdock_cli_tests
            PRIVATE
            devdock_cli
    )

    devdock_enable_warnings(
            devdock_cli_tests
    )

    add_test(
            NAME cli
            COMMAND devdock_cli_tests
    )

    add_executable(
            devdock_terminal_text_tests

            src/tests/terminal_text_tests.cpp
    )

    target_link_libraries(
            devdock_terminal_text_tests
            PRIVATE
            devdock_cli
    )

    devdock_enable_warnings(
            devdock_terminal_text_tests
    )

    add_test(
            NAME terminal_text
            COMMAND devdock_terminal_text_tests
    )

    add_executable(
            devdock_process_safety_tests

            src/tests/process_safety_tests.cpp
    )

    target_link_libraries(
            devdock_process_safety_tests
            PRIVATE
            devdock_platform
    )

    devdock_enable_warnings(
            devdock_process_safety_tests
    )

    add_test(
            NAME process_safety
            COMMAND devdock_process_safety_tests
    )

    add_executable(
            devdock_platform_tests

            src/tests/platform_tests.cpp
    )

    target_link_libraries(
            devdock_platform_tests
            PRIVATE
            devdock_platform
    )

    devdock_enable_warnings(
            devdock_platform_tests
    )

    add_test(
            NAME platform
            COMMAND devdock_platform_tests
    )

    add_executable(devdock_process_arguments_tests src/tests/process_arguments_tests.cpp)
    target_link_libraries(devdock_process_arguments_tests PRIVATE devdock_platform)
    devdock_enable_warnings(devdock_process_arguments_tests)
    add_test(NAME process_arguments COMMAND devdock_process_arguments_tests "" tail "")
    add_test(NAME process_arguments_empty_argv0 COMMAND devdock_process_arguments_tests --empty-argv0)
    set_tests_properties(process_arguments process_arguments_empty_argv0 PROPERTIES TIMEOUT 10 ENVIRONMENT "DEVDOCK_ARGUMENT_TEST=synthetic_only")
endif()
