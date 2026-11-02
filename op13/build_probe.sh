#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
set -euo pipefail
src_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
: "${ANDROID_NDK_HOME:=/opt/android-ndk}"
compiler="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android30-clang"
output=${1:-"$src_dir/../out/op13/stock-probe"}
mkdir -p -- "$(dirname -- "$output")"
"$compiler" -static -O2 -Wall -Wextra -Werror -fstack-protector-strong \
    "$src_dir/stock_probe.c" -o "$output"
file -- "$output"
