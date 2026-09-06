# DevDock

A command-line tool for macOS that shows which processes are listening on TCP ports, and stops them safely.

The usual reason to reach for it: a dev server is still holding port 3000 and you want it gone without hunting through `lsof` output for a PID.

```
$ devdock ports
PORT    PROTO   PID       PROCESS                 ADDRESS
3000    TCP     21563     node                    0.0.0.0
5432    TCP     69198     com.docker.backend      0.0.0.0
6379    TCP     69198     com.docker.backend      0.0.0.0
```

## Commands

```
devdock ports                        List every listening TCP port
devdock port <port>                  Show details for one port
devdock kill <port> [--force]        Stop the process owning a port
devdock kill --pid <pid> [--force]   Stop a process by PID
devdock --help                       Usage (also -h)
```

`devdock port <port>` reports the owning process in full, including its command line and working directory:

```
$ devdock port 3000
Port       3000
Protocol   TCP
Address    0.0.0.0
PID        21563
Process    node
Command    node server.js
CWD        /Users/me/projects/my-app
```

Command output preserves empty arguments as `''`. Unavailable or malformed argument data is shown as `<unavailable>`.

`kill` sends `SIGTERM` and waits for the process to exit. `--force` sends `SIGKILL` instead.

## Safety

Stopping a process by PID has an inherent race: the process can exit between the moment you look it up and the moment the signal is sent, and macOS may hand that PID to something else in between. Signalling blindly can kill an unrelated process.

DevDock records a process identity — PID, owning UID, and start time — and re-checks it immediately before signalling. If any part has changed, the operation is refused with `target_changed` rather than risking the wrong target.

Destructive commands also refuse to:

- run as root
- stop PID 0 or 1
- stop DevDock itself
- stop a process owned by another user

## Visibility

DevDock inspects only the processes your user account is allowed to inspect, so a
port held by another user — a system daemon, for example — is not listed. When a
port has no listener that DevDock can see, it says so and reports how much of the
scan was invisible:

```
$ devdock port 22
devdock: No visible process is listening on port 22. 255 of 617 processes could not be inspected by this user.
```

A scan is a best-effort walk over the process list, not an atomic snapshot: a
process that exits during the scan is simply skipped.

## Exit codes

| Code | Meaning |
|------|---------|
| 0 | Success |
| 1 | General failure |
| 2 | Invalid arguments |
| 3 | Target not found |
| 4 | Permission denied, or a protected process |

Useful in scripts: `devdock port 3000 >/dev/null || echo "port is free"`.

## Requirements

- macOS (Apple Silicon or Intel)
- CMake 3.25+
- Ninja
- A C++23 compiler — AppleClang 17 from the Xcode Command Line Tools works

```
brew install cmake ninja
```

Linux is not supported. The port and process lookups are built on `libproc`, and only the macOS adapters exist.

## Build

```
cmake -DCMAKE_BUILD_TYPE=Release -G Ninja -S . -B build
cmake --build build
```

## Install

```
cmake --install build --prefix "$HOME/.local"
```

Installs to `$HOME/.local/bin/devdock`. Make sure that directory is on your `PATH`. Any other prefix works — `--prefix /usr/local` needs `sudo`.

To uninstall, delete the binary. There is no uninstall target.

## Tests

Tests are built by default and run through CTest:

```
cmake --build build
cd build && ctest --output-on-failure
```

CTest covers application logic, CLI parsing and formatting, terminal text, process safety, platform adapters, and argument decoding (including empty `argv[0]`). The `platform` suite forks real child processes and signals them, so it touches the live system; it skips its destructive cases when running as root.

Pass `-DBUILD_TESTING=OFF` at configure time to skip building them.

## Development

Formatting is defined by `.clang-format` and applied by a script:

```
./format.sh           # format every source file in place
./format.sh --check   # report violations without writing
```

Requires `clang-format` (`brew install llvm`). The script finds it on `PATH` or falls back to `/opt/homebrew/opt/llvm/bin`.

Linting uses `clang-tidy`, configured by `.clang-tidy`:

```
./lint.sh         # report findings for every source file
./lint.sh --fix   # apply the automatic fixes
```

The script creates its own `build-tidy/` directory on first run. Arguments are passed
through to `clang-tidy`. After `--fix`, run `./format.sh` — clang-tidy's edits do not
follow `.clang-format`.

clang-tidy can also run as part of a build, which reports findings inline with
compilation:

```
cmake -DDEVDOCK_ENABLE_CLANG_TIDY=ON -G Ninja -S . -B build-tidy
cmake --build build-tidy
```

It is off by default, so an ordinary build never depends on clang-tidy being installed.

## Layout

The code follows a ports-and-adapters shape, so the platform-specific parts stay at the edges.

```
src/
  domain/       Core types: Process, ListeningPort, ProcessIdentity, Error
  ports/        Interfaces: PortInspector, ProcessInspector, ProcessController
  use_cases/    Application logic: ListPorts, InspectPort, KillProcess
  platform/mac/ macOS adapters built on libproc
  cli/          Argument parsing, output formatting, terminal styling
  tests/        Test executables, one per suite
```

`use_cases/` depends only on `domain/` and `ports/`, never on `platform/`. Supporting another operating system means adding adapters under `platform/` and a branch in `CMakeLists.txt`; nothing above that layer changes.
