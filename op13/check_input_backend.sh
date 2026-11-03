#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
set -euo pipefail
repo=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
workspace=$(dirname -- "$repo")
export SYSROOT=${SYSROOT:-"$repo/build/work/op13-sysroot"}
output=${1:-"$workspace/out/op13-input-check"}
vendor="$workspace/work/inspect/vendor-lib64"
headers="$repo/build/work/openharmony/drivers_peripheral/input/interfaces/include"
mkdir -p "$output"
bash "$repo/build/ohos-clang" -O2 -Wall -Wextra -Werror -I"$headers" \
  "$repo/op13/input_backend_test.c" -L"$vendor" -lhdi_input_udriver.z \
  -lhdf_utils.z -lutils.z -lhilog -lc++ -lc -o "$output/input-backend-test"
touch "$output/namespace-marker"
timeout 20 bwrap --die-with-parent --unshare-all --tmpfs / --proc /proc \
  --dev /dev --dir /dev/input --tmpfs /tmp --dir /etc \
  --ro-bind "$output/namespace-marker" /hmos-input-test-namespace \
  --ro-bind "$output" /test --ro-bind "$SYSROOT" /runtime \
  --ro-bind "$vendor" /vendor-libs --ro-bind "$workspace/work/qemu-aarch64-static" /qemu \
  /qemu -L /runtime -E LD_LIBRARY_PATH=/vendor-libs:/runtime/usr/lib /test/input-backend-test
