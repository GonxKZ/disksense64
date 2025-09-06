#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR=${BUILD_DIR:-build-tidy}

cmake -S . -B "$BUILD_DIR" -G Ninja -DENABLE_GUI=OFF -DBUILD_CLI_ONLY=ON -DENABLE_TESTING=OFF -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

FILES=$(git ls-files 'core/ops/*.cpp' 'core/scan/*.cpp' 'platform/util/*.cpp')
if ! command -v clang-tidy >/dev/null 2>&1; then
  echo "clang-tidy not found" >&2
  exit 2
fi

fail=0
for f in $FILES; do
  echo "[tidy] $f"
  clang-tidy "$f" -p "$BUILD_DIR" || fail=1
done

exit $fail

