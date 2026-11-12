#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
APP_NAME="Moonfin"
APP_ID="io.moonfin.moonfin"
BUILD_DIR="$REPO_ROOT/build/linux/release/bundle"
TEMP_DIR="$REPO_ROOT/build/linux/temp"

get_artifact_platform() {
  case "$(uname -m)" in
    x86_64) printf '%s\n' "Linux" ;;
    aarch64|arm64) printf '%s\n' "LinuxARM64" ;;
    *) printf '%s\n' "Linux_$(uname -m)" ;;
  esac
}

PLATFORM_TAG="$(get_artifact_platform)"

get_app_version() {
  grep '^version:' "$REPO_ROOT/pubspec.yaml" | sed 's/version:[[:space:]]*//' | cut -d'+' -f1 | tr -d '[:space:]'
}

resolve_mpv_package_name() {
  # Package naming differs across Ubuntu bases. Prefer newer naming when available.
  if command -v apt-cache >/dev/null 2>&1; then
    if apt-cache show libmpv2 >/dev/null 2>&1; then
      printf '%s\n' "libmpv2"
      return 0
    fi
    if apt-cache show libmpv1 >/dev/null 2>&1; then
      printf '%s\n' "libmpv1"
      return 0
    fi
  fi

  # Conservative fallback for older Ubuntu/core22 environments.
  printf '%s\n' "libmpv1"
}

resolve_build_dir() {
  local candidates=(
    "$REPO_ROOT/build/linux/x64/release/bundle"
    "$REPO_ROOT/build/linux/arm64/release/bundle"
    "$REPO_ROOT/build/linux/release/bundle"
  )

  local candidate
  for candidate in "${candidates[@]}"; do
    if [ -d "$candidate" ] && [ -f "$candidate/moonfin" ]; then
      printf '%s\n' "$candidate"
      return 0
    fi
  done

  echo "Error: Linux bundle directory not found after Flutter build." >&2
  echo "Checked:" >&2
  for candidate in "${candidates[@]}"; do
    echo "  - $candidate" >&2
  done
  exit 1
}

resolve_shared_lib() {
  local soname="$1"

  # Prefer distro packages to avoid /usr/local builds that dpkg-query cannot map.
  if command -v dpkg-query >/dev/null 2>&1; then
    local pkg_candidates=()
    case "$soname" in
      # Prefer libmpv2 where available; keep libmpv1 as distro compatibility fallback.
      libmpv.so.2) pkg_candidates=("libmpv2" "libmpv1") ;;
      libmpv.so.1) pkg_candidates=("libmpv1" "libmpv2") ;;
      libmpv.so) pkg_candidates=("libmpv-dev" "libmpv2" "libmpv1") ;;
      libsecret-1.so.0) pkg_candidates=("libsecret-1-0") ;;
      libsqlite3.so.0) pkg_candidates=("libsqlite3-0") ;;
      libwebkit2gtk-4.1.so.0) pkg_candidates=("libwebkit2gtk-4.1-0") ;;
      libwebkit2gtk-4.0.so.37) pkg_candidates=("libwebkit2gtk-4.0-37") ;;
    esac

    local dpkg_pkg
    for dpkg_pkg in "${pkg_candidates[@]:-}"; do
      local pkg_path
      pkg_path="$(dpkg-query -L "$dpkg_pkg" 2>/dev/null | awk -v s="$soname" 'index($0, "/" s) {print; exit}')"
      if [ -n "$pkg_path" ] && [ -f "$pkg_path" ]; then
        printf '%s\n' "$pkg_path"
        return 0
      fi
    done
  fi

  if command -v ldconfig >/dev/null 2>&1; then
    local resolved
    while IFS= read -r resolved; do
      [ -z "$resolved" ] && continue
      [ ! -f "$resolved" ] && continue
      case "$resolved" in
        /usr/local/*)
          continue
          ;;
      esac
      printf '%s\n' "$resolved"
      return 0
    done < <(ldconfig -p 2>/dev/null | awk -v name="$soname" '$1 == name {print $NF}')
  fi

  local candidate
  for candidate in \
    "/usr/lib/$soname" \
    "/usr/lib64/$soname" \
    "/lib/$soname" \
    "/lib64/$soname" \
    "/usr/lib/x86_64-linux-gnu/$soname" \
    "/usr/lib/aarch64-linux-gnu/$soname"; do
    if [ -f "$candidate" ]; then
      printf '%s\n' "$candidate"
      return 0
    fi
  done

  # Last resort for hosts without distro mpv packages, such as immutable distros
  # where libmpv comes from Homebrew. Transitive deps are bundled via ldd, so a
  # brew lib resolves into a self-contained bundle the same way a distro lib does.
  if command -v brew >/dev/null 2>&1; then
    candidate="$(brew --prefix 2>/dev/null)/lib/$soname"
    if [ -f "$candidate" ]; then
      printf '%s\n' "$candidate"
      return 0
    fi
  fi

  return 1
}

runtime_seed_libs() {
  local seed_libs=""

  local libmpv_path=""
  local mpv_soname
  for mpv_soname in libmpv.so.2 libmpv.so.1 libmpv.so; do
    if libmpv_path="$(resolve_shared_lib "$mpv_soname")"; then
      break
    fi
  done
  if [ -z "$libmpv_path" ]; then
    echo "Error: could not resolve libmpv (.so.2 or .so.1). Install libmpv2 (preferred) or libmpv1 on the build host." >&2
    return 1
  fi
  seed_libs="$libmpv_path"

  local libsecret_path
  if ! libsecret_path="$(resolve_shared_lib libsecret-1.so.0)"; then
    echo "Error: could not resolve libsecret-1.so.0. Install libsecret-1-0 on the build host." >&2
    return 1
  fi
  seed_libs="$seed_libs"$'\n'"$libsecret_path"

  local libsqlite3_path
  if ! libsqlite3_path="$(resolve_shared_lib libsqlite3.so.0)"; then
    echo "Error: could not resolve libsqlite3.so.0. Install libsqlite3-0 on the build host." >&2
    return 1
  fi
  seed_libs="$seed_libs"$'\n'"$libsqlite3_path"

  local webkit_path=""
  local webkit_soname
  for webkit_soname in libwebkit2gtk-4.1.so.0 libwebkit2gtk-4.0.so.37; do
    if webkit_path="$(resolve_shared_lib "$webkit_soname")"; then
      break
    fi
  done
  if [ -z "$webkit_path" ]; then
    echo "Error: could not resolve WebKitGTK runtime library (4.1 or 4.0). Install libwebkit2gtk-4.1-0 or libwebkit2gtk-4.0-37 on the build host." >&2
    return 1
  fi
  seed_libs="$seed_libs"$'\n'"$webkit_path"

  # WebKitGTK links libharfbuzz-icu, a split-out shim that minimal/immutable distros
  # (SteamOS, Fedora-atomic) do not ship. The main harfbuzz stack is skipped as a
  # system lib, so bundle the ICU shim explicitly. Best-effort: not every build host
  # has it, and it is only needed where WebKit is bundled.
  local harfbuzz_icu_path
  if harfbuzz_icu_path="$(resolve_shared_lib libharfbuzz-icu.so.0)"; then
    seed_libs="$seed_libs"$'\n'"$harfbuzz_icu_path"
  else
    echo "Warning: libharfbuzz-icu.so.0 not found on build host; AppImage may fail on distros lacking it." >&2
  fi

  printf '%s\n' "$seed_libs" | grep -v '^$' || true
}

ensure_sqlite_unversioned_link() {
  local dest_lib="$1"
  mkdir -p "$dest_lib"

  if [ -e "$dest_lib/libsqlite3.so" ]; then
    return 0
  fi

  local sqlite_real
  sqlite_real="$(find "$dest_lib" -maxdepth 1 -type f -name 'libsqlite3.so.*' | sort -V | tail -n1 || true)"
  if [ -z "$sqlite_real" ]; then
    return 0
  fi

  ln -sf "$(basename "$sqlite_real")" "$dest_lib/libsqlite3.so"
}

# Arch ships libmpv.so.2; Ubuntu 22.04 ships libmpv.so.1. Ensure both soname
# symlinks exist in the bundle so binaries compiled against either version work.
ensure_mpv_compat_symlinks() {
  local dest_lib="$1"

  local mpv_real
  mpv_real="$(find "$dest_lib" -maxdepth 1 -name 'libmpv.so.*' ! -type l 2>/dev/null | sort -V | tail -n1 || true)"
  [ -z "$mpv_real" ] && return 0

  local base
  base="$(basename "$mpv_real")"

  # A bare "[ ! -e ] && ln" would return 1 when every symlink already exists,
  # which under set -e kills the build with no error message.
  for alt in libmpv.so.1 libmpv.so.2 libmpv.so; do
    if [ ! -e "$dest_lib/$alt" ]; then
      ln -sf "$base" "$dest_lib/$alt"
    fi
  done
  return 0
}

collect_transitive_libs() {
  local seed_libs="$1"
  local all_deps=""

  while IFS= read -r seed; do
    [ -z "$seed" ] && continue
    [ ! -f "$seed" ] && continue

    local deps
    deps="$(ldd "$seed" 2>/dev/null | grep -oP '=> \K/[^ ]+' || true)"
    # Libraries built without a SONAME (brew mujs) appear in ldd as a bare
    # absolute path with no arrow, so collect those too.
    local abs_deps
    abs_deps="$(ldd "$seed" 2>/dev/null | grep -oP '^\s*\K/\S+(?=\s+\(0x)' || true)"
    all_deps="$(printf '%s\n%s\n%s\n%s' "$all_deps" "$deps" "$abs_deps" "$seed")"
  done <<< "$seed_libs"

  printf '%s\n' "$all_deps" | sort -u | grep -v '^$' || true
}

# Libraries left out of the bundle because the target system has to own them,
# either because they talk to the kernel and the display stack or because a
# bundled copy would shadow the matching system one.
#
# Read this as the complete list of what we ask a machine to already have.
# Distros vary from shipping almost everything to shipping almost nothing, so
# anything not named here has to travel with the app. libselinux is the example
# to remember: Ubuntu builds glib against it so it reaches the dependency graph
# on the build host, while Arch does not install it, and leaving it out meant
# the app refused to start there. A machine having the library is not enough
# either: openSUSE builds ncurses without the versioned symbols Ubuntu adds, so
# the bundled libcaca could not resolve them and the app refused to start there
# too.
#
# Entries match anywhere in the file name, so a name that is a prefix of
# another one has to say so. Asking for libcrypt used to hand libcrypto to the
# host as well, which left the bundle carrying libssl and expecting an OpenSSL
# it was never built against.
runtime_skip_pattern() {
  local skip='linux-vdso|ld-linux|libc[.]so|libm[.]so|libpthread|libdl[.]so|librt[.]so'
  skip="$skip"'|libstdc[+][+]|libgcc_s'
  skip="$skip"'|libX[a-z]|libxcb|libxkb|libxshmfence|libICE|libSM'
  skip="$skip"'|libwayland|libffi|libpcre'
  skip="$skip"'|libGL[.]so|libGLX|libGLdispatch|libGLESv|libEGL|libOpenGL'
  skip="$skip"'|libdrm|libgbm'
  skip="$skip"'|libglib|libgobject|libgio|libgmodule|libgthread'
  skip="$skip"'|libpulse|libdbus|libasound|libsndfile|libpipewire|libspa|libjack'
  skip="$skip"'|libfontconfig|libfreetype|libharfbuzz|libcairo|libpango|libpixman'
  skip="$skip"'|libatk|libgdk|libgtk|libepoxy|libgdk_pixbuf|librsvg'
  skip="$skip"'|libmount|libblkid|libuuid|libresolv|libnss3|libnssutil'
  skip="$skip"'|libsystemd'
  # OpenSSL finds its trust store and its provider modules through a directory
  # fixed when it was built, so a copy carried from another distro can stop
  # verifying certificates. Both halves stay with the host for that reason, and
  # they stay together: a bundled libssl against a system libcrypto is the
  # mismatch that has to be avoided.
  skip="$skip"'|libssl[.]so|libcrypto[.]so'
  printf '%s\n' "$skip"
}

# Libraries linked against a no-SONAME dependency carry its absolute build-host
# path in DT_NEEDED, which the loader uses verbatim on the target machine.
# Rewrite those entries to bare basenames so the bundled copy is found instead.
fix_absolute_needed() {
  local dest_lib="$1"

  if ! command -v patchelf >/dev/null 2>&1; then
    echo "Warning: patchelf not found; absolute DT_NEEDED paths were not rewritten." >&2
    return 0
  fi

  local lib abs base
  for lib in "$dest_lib"/*.so*; do
    [ -f "$lib" ] || continue
    [ -L "$lib" ] && continue
    while IFS= read -r abs; do
      [ -z "$abs" ] && continue
      base="$(basename "$abs")"
      # Brew ships libraries read-only and cp -L keeps that mode.
      chmod u+w "$lib" 2>/dev/null || true
      patchelf --replace-needed "$abs" "$base" "$lib" 2>/dev/null || true
      if [ ! -e "$dest_lib/$base" ] && [ -f "$abs" ]; then
        cp -L "$abs" "$dest_lib/$base"
      fi
    done < <(readelf -d "$lib" 2>/dev/null | sed -n 's/.*(NEEDED).*\[\(\/[^]]*\)\].*/\1/p')
  done
}

copy_runtime_libs() {
  local dest_lib="$1"
  local lib_paths="$2"
  local skip_pattern="$3"

  mkdir -p "$dest_lib"

  local count=0
  while IFS= read -r lib_path; do
    [ -z "$lib_path" ] && continue
    [ ! -f "$lib_path" ] && continue

    local lib_name
    lib_name="$(basename "$lib_path")"
    # libharfbuzz-icu is matched by the libharfbuzz skip entry but must be bundled
    # (WebKit needs it; it is absent on minimal/immutable distros). Keep plain
    # libharfbuzz and the other skips.
    if [[ ! "$lib_name" =~ ^libXpresent[.]so ]] && [[ ! "$lib_name" =~ ^libharfbuzz-icu[.]so ]]; then
      [[ "$lib_name" =~ $skip_pattern ]] && continue
    fi

    local real_path
    real_path="$(readlink -f "$lib_path")"
    [ -f "$real_path" ] || continue

    local real_name
    real_name="$(basename "$real_path")"

    if [ ! -f "$dest_lib/$real_name" ]; then
      cp -L "$real_path" "$dest_lib/$real_name"
      count=$((count + 1))
    fi

    if [ "$real_name" != "$lib_name" ] && [ ! -e "$dest_lib/$lib_name" ]; then
      ln -sf "$real_name" "$dest_lib/$lib_name"
    fi

    local soname
    soname="$(readelf -d "$real_path" 2>/dev/null | sed -n 's/.*SONAME.*\[\(.*\)\].*/\1/p' | head -n1)"
    if [ -n "$soname" ] && [ "$soname" != "$real_name" ] && [ ! -e "$dest_lib/$soname" ]; then
      ln -sf "$real_name" "$dest_lib/$soname"
    fi
  done <<< "$lib_paths"

  printf '%s\n' "$count"
}

inject_linux_runtime_libs() {
  local bundle_dir="$1"

  local seed_libs
  seed_libs="$(runtime_seed_libs)"
  if [ -z "$seed_libs" ]; then
    echo "Warning: Could not resolve runtime seed libraries on this build host." >&2
    return 1
  fi

  local all_deps
  all_deps="$(collect_transitive_libs "$seed_libs")"

  local skip
  skip="$(runtime_skip_pattern)"

  local bundled_count
  bundled_count="$(copy_runtime_libs "$bundle_dir/lib" "$all_deps" "$skip")"
  fix_absolute_needed "$bundle_dir/lib"
  ensure_sqlite_unversioned_link "$bundle_dir/lib"
  ensure_mpv_compat_symlinks "$bundle_dir/lib"
  echo "Bundled $bundled_count runtime libraries for AppImage/Tarball"

  verify_host_dependencies "$bundle_dir"
}

# Names every library the finished bundle still expects to find on the machine
# it runs on, which is the binary's and the bundled libraries' NEEDED entries
# minus whatever travels in the bundle.
host_dependencies() {
  local bundle_dir="$1"
  local file
  for file in "$bundle_dir"/* "$bundle_dir"/lib/*.so*; do
    [ -f "$file" ] || continue
    [ -L "$file" ] && continue
    readelf -d "$file" 2>/dev/null |
      sed -n 's/.*(NEEDED).*\[\([^]]*\)\].*/\1/p'
  done | sort -u | while IFS= read -r name; do
    [ -n "$name" ] || continue
    [ -e "$bundle_dir/lib/$name" ] && continue
    printf '%s\n' "$name"
  done
}

# The skip list is a promise that a machine already has those libraries. This
# checks the finished bundle keeps that promise, so a dependency that slipped
# past the collector shows up on the build host rather than on a user's.
verify_host_dependencies() {
  local bundle_dir="$1"
  local skip deps unexpected=""

  if ! command -v readelf >/dev/null 2>&1; then
    echo "Warning: readelf not found, host dependencies were not checked." >&2
    return 0
  fi

  skip="$(runtime_skip_pattern)"
  deps="$(host_dependencies "$bundle_dir")"

  echo "Bundle expects these from the host:"
  printf '  %s\n' $deps

  local name
  while IFS= read -r name; do
    [ -n "$name" ] || continue
    [[ "$name" =~ $skip ]] && continue
    unexpected="${unexpected}  ${name}"$'\n'
  done <<< "$deps"

  if [ -n "$unexpected" ]; then
    echo "Error: these are neither bundled nor listed as system provided:" >&2
    printf '%s' "$unexpected" >&2
    echo "Bundle them, or add them to runtime_skip_pattern if every distro has them." >&2
    return 1
  fi
}

# Bundle libmpv + transitive deps for Flatpak (sandboxed, no system libs available).
inject_flatpak_libs() {
  local dest_lib="$1"
  mkdir -p "$dest_lib"
  echo "Bundling runtime libraries for Flatpak..."

  local seed_libs
  seed_libs="$(runtime_seed_libs)"
  if [ -z "$seed_libs" ]; then
    echo "Error: could not resolve required runtime seed libraries." >&2
    return 1
  fi

  local all_deps
  all_deps="$(collect_transitive_libs "$seed_libs")"

  local skip
  skip="$(runtime_skip_pattern)"

  local count
  count="$(copy_runtime_libs "$dest_lib" "$all_deps" "$skip")"
  fix_absolute_needed "$dest_lib"
  ensure_sqlite_unversioned_link "$dest_lib"
  ensure_mpv_compat_symlinks "$dest_lib"

  echo "Bundled $count runtime libraries for Flatpak"
}


required_glibc_version() {
  local tree="$1" file

  command -v readelf >/dev/null 2>&1 || return 0

  for file in "$tree"/* "$tree"/lib/*.so*; do
    [ -f "$file" ] || continue
    [ -L "$file" ] && continue
    readelf --dyn-syms -W "$file" 2>/dev/null |
      awk '$7 == "UND"' |
      grep -o 'GLIBC_[0-9][0-9.]*' || true
  done | sed 's/^GLIBC_//' | sort -uV | tail -n1
}

retry_with_backoff() {
  local max_attempts="$1"
  shift

  local attempt=1
  while true; do
    if "$@"; then
      return 0
    fi

    if [ "$attempt" -ge "$max_attempts" ]; then
      return 1
    fi

    local delay=$((attempt * 10))
    echo "Command failed (attempt $attempt/$max_attempts). Retrying in ${delay}s..."
    sleep "$delay"
    attempt=$((attempt + 1))
  done
}

main() {
  local version="$(get_app_version)"

  echo "======================================"
  echo "Moonfin Linux Package Builder - Flathub version"
  echo "Version: ${version}"
  echo "======================================"
  echo ""

  BUILD_DIR="$(resolve_build_dir)"
  inject_flatpak_libs "/lib/moonfin"
  if [ "${BUNDLE_RUNTIME_LIBS:-0}" = "1" ]; then
    inject_linux_runtime_libs "$BUILD_DIR"
  else
    echo "Skipping bundled runtime libs (set BUNDLE_RUNTIME_LIBS=1 to enable)."
  fi
  rm -rf "$TEMP_DIR"

  echo ""
  echo "======================================"
  echo "Build complete!"
  echo "Artifacts: $REPO_ROOT"
  ls -lh "$REPO_ROOT"/Moonfin_* 2>/dev/null || echo "(no artifacts built)"
  echo "======================================"
}

main "$@"
