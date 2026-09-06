#!/bin/sh
# Runs clang-tidy over all C++ sources. Pass --fix to apply automatic fixes.
set -eu

cd "$(dirname "$0")"

CLANG_TIDY="${CLANG_TIDY:-$(command -v clang-tidy || echo /opt/homebrew/opt/llvm/bin/clang-tidy)}"

if [ ! -x "$CLANG_TIDY" ]; then
    echo "clang-tidy not found. Install it with: brew install llvm" >&2
    exit 1
fi

BUILD_DIR=build-tidy

if [ ! -f "$BUILD_DIR/compile_commands.json" ]; then
    cmake -DCMAKE_BUILD_TYPE=Debug -G Ninja -S . -B "$BUILD_DIR" >/dev/null
fi

# CMake omits -isysroot when AppleClang uses the default SDK, so clang-tidy
# cannot find the libc++ headers without it.
SYSROOT="$(xcrun --show-sdk-path)"

find src -type f -name '*.cpp' -print0 |
    xargs -0 "$CLANG_TIDY" -p "$BUILD_DIR" --quiet --extra-arg=-isysroot"$SYSROOT" "$@"
