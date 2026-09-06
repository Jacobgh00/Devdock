#!/bin/sh
# Formats all C++ sources with clang-format. Pass --check to report without writing.
set -eu

cd "$(dirname "$0")"

CLANG_FORMAT="${CLANG_FORMAT:-$(command -v clang-format || echo /opt/homebrew/opt/llvm/bin/clang-format)}"

if [ ! -x "$CLANG_FORMAT" ]; then
    echo "clang-format not found. Install it with: brew install llvm" >&2
    exit 1
fi

if [ "${1:-}" = "--check" ]; then
    find src -type f \( -name '*.cpp' -o -name '*.hpp' \) -print0 |
        xargs -0 "$CLANG_FORMAT" --dry-run -Werror
    echo "All files are formatted."
else
    find src -type f \( -name '*.cpp' -o -name '*.hpp' \) -print0 |
        xargs -0 "$CLANG_FORMAT" -i
    echo "Formatted $(find src -type f \( -name '*.cpp' -o -name '*.hpp' \) | wc -l | tr -d ' ') files."
fi
