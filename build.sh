#!/bin/sh -eu

set -xeuo pipefail

patch -p1 --directory=third_party/node < third_party/node/patches/lit_html.patch
patch -p1 --directory=third_party/node < third_party/node/patches/typescript.patch
patch -p1 --directory=third_party/node/node_modules/@types/d3 < third_party/node/patches/chromium_d3_types_index.patch
patch -p1 --directory=third_party/node < third_party/node/patches/types_chai.patch
patch -p1 --directory=third_party/node < third_party/node/patches/ts_poet.patch
patch -p1 --directory=third_party/node < third_party/node/patches/types_trusted_types.patch

./build/util/lastchange.py -m SKIA_COMMIT_HASH -s third_party/skia --header skia/ext/skia_commit_hash.h
/usr/bin/env CC=gcc CXX=g++ python3 gn/build/gen.py
/usr/bin/env CC=gcc CXX=g++ ninja -C gn/out -j $FLATPAK_BUILDER_N_JOBS

# symlink llvm for chromium to use
mkdir -p third_party/llvm-build
ln -s /usr/lib/sdk/llvm21 $FLATPAK_BUILDER_BUILDDIR/third_party/llvm-build/Release+Asserts

#https://github.com/flathub/io.github.ungoogled_software.ungoogled_chromium/blob/3673d6bfd7d8947bd0736e49cdc8c738d0f07bfb/build-aux/build.sh#L55
mkdir -p $FLATPAK_BUILDER_BUILDDIR/bindgen/bin
ln -svf "$(command -v bindgen)" $FLATPAK_BUILDER_BUILDDIR/bindgen/bin/bindgen
ln -svf "${LIBCLANG_PATH}" -t $FLATPAK_BUILDER_BUILDDIR/bindgen

export GN_DEFINES="angle_build_tests=false \
angle_enable_commit_id=false \
blink_symbol_level=0 \
build_angle_perftests=false \
build_dawn_tests=false \
build_with_tflite_lib=false \
chrome_enable_logging_by_default=false \
chrome_pgo_phase=0 \
clang_base_path=/usr/lib/sdk/llvm21 \
clang_version=21 \
dawn_enable_null=false \
dcheck_always_on=false \
devtools_skip_typecheck=false \
disable_file_support=false \
disable_histogram_support=true \
enable_background_mode=false \
enable_basic_print_dialog=false \
enable_browser_speech_service=false \
enable_concurrent_basic_print_dialogs=false \
enable_media_remoting=false \
enable_media_remoting_rpc=false \
enable_nocompile_tests=false \
enable_pdf_ink2=false \
enable_pdf_save_to_drive=false \
enable_perfetto_unittests=false \
enable_rlz=false \
enable_rust_png=false \
enable_screen_ai_browsertests=false \
enable_service_discovery=false \
enable_speech_service=false \
enable_trace_logging=false \
enable_vr=false \
enable_widevine=false \
generate_about_credits=false \
gtest_enable_absl_printers=false \
headless_enable_commands=false \
headless_mode_policy_supported=false \
headless_use_policy=false \
headless_use_prefs=false \
host_toolchain=//build/toolchain/linux/unbundle:default \
icu_use_data_file=false \
include_branded_entitlements=false \
init_stack_vars=false \
is_official_build=true \
is_debug=false \
media_use_openh264=false \
optional_trace_events_enabled=false \
ozone_platform_headless=false \
proprietary_codecs=false \
removed_rust_stdlib_libs=[unicode_width] \
rust_bindgen_root=$FLATPAK_BUILDER_BUILDDIR/bindgen \
rustc_version=\"$(rustc --version)\" \
rust_sysroot_absolute=/usr/lib/sdk/rust-stable \
safe_browsing_mode=0 \
skia_enable_skshaper_tests=false \
tint_build_unittests=false \
treat_warnings_as_errors=false \
use_sysroot=false \
use_system_freetype=true \
use_system_harfbuzz=true \
use_system_lcms2=true \
use_system_libjpeg=true \
use_system_libopenjpeg2=true \
use_system_libpng=true \
use_system_libtiff=true \
use_system_libwayland=true \
use_system_minigbm=true \
use_system_zlib=true \
v8_deprecation_warnings=false \
v8_enable_test_features=false \
v8_enable_webassembly=true \
v8_imminent_deprecation_warnings=false \
"

/usr/bin/env python3 cef/tools/gclient_hook.py

# From ungoogled chromium flatpak
patch -p1 < flatpak-Adjust-paths-for-the-sandbox.patch
patch -p1 --directory=third_party/angle < angle-remove-undefined-const.patch
patch -p1 --directory=ungoogled-chromium < ungoogled-chromium-adjust-for-cef.patch
patch -p1 --directory=ungoogled-chromium < ungoogled-chromium-ignore-nonexistent-binaries.patch
patch -p1 --directory=ungoogled-chromium < ungoogled-chromium-remove-extra-locales.patch

./ungoogled-chromium/utils/prune_binaries.py . ungoogled-chromium/pruning.list
./ungoogled-chromium/utils/patches.py apply . ungoogled-chromium/patches
./ungoogled-chromium/utils/domain_substitution.py apply -r ungoogled-chromium/domain_regex.list -f ungoogled-chromium/domain_substitution.list -c domsubcache.tar.gz .

#Use system node
mkdir -p third_party/node/linux/node-linux-x64/bin
ln -sfn /usr/lib/sdk/node22/bin/node third_party/node/linux/node-linux-x64/bin/node

/usr/bin/env ninja -C $FLATPAK_BUILDER_BUILDDIR/out/Release_GN -j $FLATPAK_BUILDER_N_JOBS libcef chrome_sandbox
python3 ./cef/tools/make_distrib.py --ninja-build --minimal --no-docs --no-archive --output-dir=/app/cef
