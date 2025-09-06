#!/usr/bin/env bash
set -euo pipefail

shopt -s globstar

FILES=$(git ls-files '**/*.c' '**/*.cpp' '**/*.h' '**/*.hpp' '**/*.cc' '**/*.hh' '**/*.cxx' '**/*.ixx' '**/*.mm' '**/*.m' '**/*.qml' '**/*.cmake' 'CMakeLists.txt')

if ! command -v clang-format >/dev/null 2>&1; then
  echo "clang-format not found" >&2
  exit 2
fi

fail=0
for f in $FILES; do
  clang-format --dry-run --Werror "$f" || fail=1
done

if [ "$fail" -ne 0 ]; then
  echo "Code style check failed. Run clang-format to fix." >&2
  exit 1
fi
echo "Code style OK"
