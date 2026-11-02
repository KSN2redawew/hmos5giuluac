#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# Build only the OP13 OHOS display layer; never change donor images/patches.
set -euo pipefail
repo=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
export SYSROOT=${SYSROOT:-"$repo/build/work/op13-sysroot"}
oh_src=${OH_SRC:-"$repo/build/work/openharmony"}
display_interfaces=${DISPLAY_INTERFACES:-"$repo/build/work/ohos-5.0.3"}
output=${1:-"$repo/../out/op13-display"}
mkdir -p "$output/obj"
source_dir="$repo/op13/display"
includes=(
 "$source_dir/common" "$source_dir/hdi-gen" "$source_dir/hdi-gen/display"
 "$source_dir/hdi-gen/display/buffer" "$source_dir/hdi-gen/display/composer"
 "$source_dir/hdi-gen/display/graphic/common"
 "$display_interfaces/drivers_interface/display/composer/hdifd_parcelable"
 "$display_interfaces/drivers_interface/display/composer" "$display_interfaces/drivers_interface/display"
 "$display_interfaces/drivers_interface/display/composer/common/include"
 "$display_interfaces/drivers_peripheral/display/buffer/hdi_service/include"
 "$display_interfaces/drivers_peripheral/display/composer/hdi_service/include"
 "$display_interfaces/drivers_peripheral/display/utils/include"
 "$oh_src/graphic_graphic_surface/interfaces/inner_api/utils"
 "$oh_src/hdf_core/interfaces/inner_api/utils"
 "$oh_src/hdf_core/interfaces/inner_api/osal/shared"
 "$oh_src/hdf_core/interfaces/inner_api/osal/uhdf"
 "$oh_src/hdf_core/interfaces/inner_api/hdi"
 "$oh_src/hdf_core/interfaces/inner_api/hdi/base"
 "$oh_src/hdf_core/interfaces/inner_api/ipc"
 "$oh_src/hdf_core/interfaces/inner_api/core"
 "$oh_src/communication_ipc/interfaces/innerkits/ipc_core/include"
 "$oh_src/commonlibrary_c_utils/base/include"
 "$oh_src/hiviewdfx_hilog/interfaces/native/innerkits/include"
 "$oh_src/hiviewdfx_hitrace/interfaces/native/innerkits/include/hitrace_meter"
 "$oh_src/third_party_libdrm" "$oh_src/third_party_libdrm/include/drm"
 "$oh_src/third_party_bounds_checking_function/include"
)
flags=(-std=c++17 -O2 -fPIC -Wno-unknown-warning-option -Wno-format -Wno-deprecated-declarations)
for include in "${includes[@]}"; do flags+=("-I$include"); done
cxx="$repo/build/ohos-clang++"
"$cxx" "${flags[@]}" -shared "$source_dir/buffer/sweet_buffer_vdi.cpp" \
 -Wl,-soname,libdisplay_buffer_vdi_impl.z.so -Wl,--no-undefined \
 -ldrm -lhilog -lc++ -lc -o "$output/libdisplay_buffer_vdi_impl.z.so"
objects=()
for source in "$source_dir"/composer/src/*.cpp; do
 object="$output/obj/$(basename "${source%.cpp}").o"
 "$cxx" "${flags[@]}" -I"$source_dir/composer/include" -c "$source" -o "$object"
 objects+=("$object")
done
"$cxx" -shared "${objects[@]}" -Wl,-soname,libdisplay_composer_vdi_impl.z.so \
 -Wl,--no-undefined -L"$output" -ldisplay_buffer_vdi_impl.z -ldrm -lhilog \
 -lhdf_utils.z -lsec_shared.z -lhitrace_meter -lutils.z -lc++ -lc \
 -o "$output/libdisplay_composer_vdi_impl.z.so"
file "$output"/*.so
"$cxx" "${flags[@]}" -I"$source_dir/composer/include" \
 "$repo/op13/display_mode_test.cpp" -L"$output" -ldisplay_composer_vdi_impl.z \
 -ldisplay_buffer_vdi_impl.z -ldrm -lhilog -lhdf_utils.z -lsec_shared.z \
 -lhitrace_meter -lutils.z -lc++ -lc -o "$output/display-mode-test"
