#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
APP_NAME="Moonfin"
APP_ID="io.moonfin.moonfin"
#APP_ICON="$REPO_ROOT/assets/icons/moonfin.png"
BUILD_DIR="$REPO_ROOT/build/linux/release/bundle"
TEMP_DIR="$REPO_ROOT/build/linux/temp"

# Artifact names carry the architecture, except on x86_64 which keeps the plain
# "Linux" name. An installed AppImage updates itself from a zsync file named
# after the artifact, and the AUR package and the README point at those same
# names, so renaming the x86_64 files would break all three.
get_artifact_platform() {
  case "$(uname -m)" in
    x86_64) printf '%s\n' "Linux" ;;
    aarch64|arm64) printf '%s\n' "LinuxARM64" ;;
    *) printf '%s\n' "Linux_$(uname -m)" ;;
  esac
}

PLATFORM_TAG="$(get_artifact_platform)"

## Snap stage-packages land under the host's multiarch directory, so the snap's
## library path has to name the architecture it is being built for.
#get_multiarch_triplet() {
#  case "$(uname -m)" in
#    aarch64|arm64) printf '%s\n' "aarch64-linux-gnu" ;;
#    *) printf '%s\n' "x86_64-linux-gnu" ;;
#  esac
#}
#
#get_snap_base() {
#  local release=""
#  if [ -r /etc/os-release ]; then
#    release="$(. /etc/os-release && printf '%s' "${VERSION_ID:-}")"
#  fi
#
#  case "$release" in
#    22.04) printf '%s\n' "core22" ;;
#    24.04) printf '%s\n' "core24" ;;
#    26.04) printf '%s\n' "core26" ;;
#    *) return 1 ;;
#  esac
#}

#get_deb_architecture() {
#  local machine
#  machine="$(uname -m)"
#
#  case "$machine" in
#    x86_64)
#      printf '%s\n' "amd64"
#      ;;
#    aarch64|arm64)
#      printf '%s\n' "arm64"
#      ;;
#    armv7l|armv7*)
#      printf '%s\n' "armhf"
#      ;;
#    i386|i486|i586|i686)
#      printf '%s\n' "i386"
#      ;;
#    *)
#      printf '%s\n' "$machine"
#      ;;
#  esac
#}

#resolve_flutter() {
#  if [ -n "${FLUTTER_BIN:-}" ] && [ -x "$FLUTTER_BIN" ]; then
#    printf '%s\n' "$FLUTTER_BIN"
#    return 0
#  fi
#
#  if command -v flutter >/dev/null 2>&1; then
#    command -v flutter
#    return 0
#  fi
#
#  local candidates=(
#    "$HOME/flutter/bin/flutter"
#    "$HOME/Documents/flutter/bin/flutter"
#    "$HOME/snap/flutter/common/flutter/bin/flutter"
#  )
#
#  local candidate
#  for candidate in "${candidates[@]}"; do
#    if [ -x "$candidate" ]; then
#      printf '%s\n' "$candidate"
#      return 0
#    fi
#  done
#
#  echo "Error: Flutter not found. Add flutter to PATH or set FLUTTER_BIN to the full flutter executable path." >&2
#  exit 1
#}

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

#create_desktop_file() {
#  local dest="$1"
#  local version="${2:-}"
#  mkdir -p "$dest"
#  cat > "$dest/${APP_ID}.desktop" << EOF
#[Desktop Entry]
#Type=Application
#Name=Moonfin
#Exec=moonfin %U
#Icon=${APP_ID}
#Categories=AudioVideo;Video;
#Comment=Jellyfin & Emby media client
#Terminal=false
#StartupWMClass=${APP_ID}
#MimeType=x-scheme-handler/moonfin;
#EOF
#  # AppImage launchers read the version from the embedded desktop entry rather
#  # than the filename.
#  if [ -n "$version" ]; then
#    echo "X-AppImage-Version=${version}" >> "$dest/${APP_ID}.desktop"
#  fi
#}
#
#create_metainfo_file() {
#  local dest="$1"
#  local version="$2"
#
#  mkdir -p "$dest"
#  cat > "$dest/${APP_ID}.metainfo.xml" << EOF
#<?xml version="1.0" encoding="UTF-8"?>
#<component type="desktop-application">
#  <id>${APP_ID}</id>
#  <metadata_license>CC0-1.0</metadata_license>
#  <project_license>GPL-3.0-only</project_license>
#  <name>Moonfin</name>
#  <developer_name>Moonfin Team</developer_name>
#  <summary>Jellyfin &amp; Emby media client</summary>
#  <description>
#    <p>Moonfin is a media client for Jellyfin and Emby servers, available on mobile, TV, and desktop platforms.</p>
#  </description>
#  <url type="homepage">https://moonfin.app/</url>
#  <launchable type="desktop-id">${APP_ID}.desktop</launchable>
#  <releases>
#    <release version="${version}" date="$(date '+%Y-%m-%d')"/>
#  </releases>
#  <content_rating type="oars-1.1"/>
#</component>
#EOF
#}

#ensure_flatpak_runtime() {
#  # GNOME runtime includes WebKitGTK required by linux webview plugin.
#  local runtime="org.gnome.Platform//51"
#  local sdk="org.gnome.Sdk//51"
#
#  if flatpak info --user "$runtime" >/dev/null 2>&1 && flatpak info --user "$sdk" >/dev/null 2>&1; then
#    return 0
#  fi
#
#  flatpak remote-add --user --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo 2>/dev/null || true
#  flatpak install --user --noninteractive flathub "$runtime" "$sdk" || true
#}

#icon_warning_shown=""
#
#copy_icon() {
#  local dest="$1"
#  if [ ! -f "$APP_ICON" ]; then
#    if [ -z "$icon_warning_shown" ]; then
#      icon_warning_shown=1
#      echo "Warning: app icon missing at $APP_ICON, packages will show a placeholder." >&2
#    fi
#    return
#  fi
#  mkdir -p "$dest"
#  cp "$APP_ICON" "$dest/${APP_ID}.png"
#}
#
## Installs the icon into the hicolor theme, the path every current desktop
## searches. On Wayland it is how a window gets an icon at all: the compositor
## resolves it from the desktop entry named by the window's app id, never from
## the window itself. share/pixmaps stays alongside for older lookups.
#install_icons() {
#  local share_dir="$1"
#  copy_icon "$share_dir/icons/hicolor/512x512/apps"
#  copy_icon "$share_dir/pixmaps"
#}

build_flutter_binary() {
  echo "Building Flutter release binary for Linux..."
  cd "$REPO_ROOT"

  local attempt=1 max_attempts=3
  while true; do
    if flutter build linux --release --no-pub --dart-define=DISTRIBUTION_CHANNEL=linux; then
      return 0
    fi
    if [ "$attempt" -ge "$max_attempts" ]; then
      echo "Error: 'flutter build linux' failed after ${max_attempts} attempts." >&2
      return 1
    fi
    echo "flutter build failed (attempt ${attempt}/${max_attempts}); purging pdfium cache and retrying..." >&2
    # The plugin downloads into <cmake-binary-dir>/pdfium and copies into .lib/.
    # Remove both across whatever arch/release dir Flutter used so the next configure
    # re-fetches from scratch instead of reusing the truncated archive.
    rm -rf "$REPO_ROOT"/build/linux/*/release/pdfium \
           "$REPO_ROOT"/build/linux/*/release/.lib/* 2>/dev/null || true
    sleep $((attempt * 10))
    attempt=$((attempt + 1))
  done
}

#build_appimage() {
#  echo "=== Building AppImage ==="
#  if ! command -v appimagetool >/dev/null 2>&1; then
#    echo "Skipping AppImage: appimagetool not found"
#    echo "  Install: https://github.com/AppImage/AppImageKit/releases"
#    return 1
#  fi
#
#  local appimage_dir="$TEMP_DIR/appimage"
#  local version="$(get_app_version)"
#  local appimage_name="${APP_NAME}_${PLATFORM_TAG}_v${version}.AppImage"
#
#  rm -rf "$appimage_dir"
#  mkdir -p "$appimage_dir"
#
#  cp -r "$BUILD_DIR"/* "$appimage_dir/"
#  inject_linux_runtime_libs "$appimage_dir"
#
#  cat > "$appimage_dir/AppRun" << 'EOF'
##!/bin/bash
#APPDIR="$(cd "$(dirname "$0")" && pwd)"
#export LD_LIBRARY_PATH="$APPDIR/lib:$LD_LIBRARY_PATH"
#
## nix-ld only hands NixOS programs their libraries once they start, and ldd
## runs Nix's own loader, which never looks there, so the check adds them.
#check_lib_path="$LD_LIBRARY_PATH${NIX_LD_LIBRARY_PATH:+:$NIX_LD_LIBRARY_PATH}"
#
#resolve_exact_lib() {
#  local lib_name="$1"
#  local old_ifs="$IFS"
#  IFS=':'
#  for lib_dir in $check_lib_path; do
#    [ -n "$lib_dir" ] && [ -e "$lib_dir/$lib_name" ] && IFS="$old_ifs" && return 0
#  done
#  IFS="$old_ifs"
#
#  for lib_dir in /lib /lib64 /usr/lib /usr/lib64 /usr/lib/x86_64-linux-gnu /usr/lib/aarch64-linux-gnu; do
#    [ -e "$lib_dir/$lib_name" ] && return 0
#  done
#
#  return 1
#}
#
#ldd_output="$(LD_LIBRARY_PATH="$check_lib_path" ldd "$APPDIR/moonfin" 2>/dev/null || true)"
#
## Only "libfoo.so => not found" lines name a missing library. Symbol version
## lines start with the binary path, so matching bare "not found" reports the
## binary itself as missing.
#missing_libs="$(printf '%s\n' "$ldd_output" | awk '$2 == "=>" && $3 == "not" && $4 == "found" {print $1}' | sort -u)"
#version_errors="$(printf '%s\n' "$ldd_output" | grep -F 'not found' | grep -v ' => ' || true)"
#
#if ! resolve_exact_lib libsqlite3.so; then
#  missing_libs="$(printf '%s\n%s\n' "$missing_libs" "libsqlite3.so" | awk 'NF' | sort -u)"
#fi
#
#if [ -n "$missing_libs" ]; then
#  echo "Moonfin cannot start. Missing shared libraries:" >&2
#  printf '  - %s\n' $missing_libs >&2
#  echo "Install the missing libraries for your distro and retry." >&2
#  exit 127
#fi
#
#if [ -n "$version_errors" ]; then
#  echo "Moonfin cannot start. A library your system provides is missing symbols" >&2
#  echo "this build needs:" >&2
#  printf '%s\n' "$version_errors" >&2
#  echo "That library has to be bundled with the app for your distro." >&2
#  echo "Please report this output so the next build carries it." >&2
#  echo "A deb, rpm, flatpak or snap build works in the meantime." >&2
#  exit 127
#fi
#
#exec "$APPDIR/moonfin" "$@"
#EOF
#  chmod +x "$appimage_dir/AppRun"
#
#  create_desktop_file "$appimage_dir" "$version"
#  copy_icon "$appimage_dir"
#  install_icons "$appimage_dir/usr/share"
#  mkdir -p "$appimage_dir/usr/share/applications"
#  cp "$appimage_dir/${APP_ID}.desktop" \
#    "$appimage_dir/usr/share/applications/${APP_ID}.desktop"
#  # Desktop integration copies the entry and the themed icon out of the image,
#  # and file managers read the icon for the file itself from .DirIcon.
#  if [ -f "$appimage_dir/${APP_ID}.png" ]; then
#    cp "$appimage_dir/${APP_ID}.png" "$appimage_dir/.DirIcon"
#  fi
#
#  # Update information lets AppImage updaters fetch only the changed blocks from
#  # the .zsync published with each release. The wildcard matches whichever
#  # version the current release carries.
#  local update_info="${APPIMAGE_UPDATE_INFO:-gh-releases-zsync|Moonfin-Client|Moonfin-Core|latest|${APP_NAME}_${PLATFORM_TAG}_v*.AppImage.zsync}"
#  local appimagetool_args=()
#  if command -v zsyncmake >/dev/null 2>&1; then
#    appimagetool_args+=( -u "$update_info" )
#  else
#    echo "Warning: zsyncmake not found, building AppImage without embedded update information." >&2
#    echo "  Install the zsync package to enable in-place updates." >&2
#  fi
#
#  cd "$TEMP_DIR"
#  # VERSION stamps the build. Passing an explicit output name keeps the
#  # artifact filename unchanged.
#  if ! VERSION="$version" appimagetool "${appimagetool_args[@]}" "$appimage_dir" "$appimage_name"; then
#    echo "Error: appimagetool failed." >&2
#    return 1
#  fi
#
#  if [ -f "$appimage_name" ]; then
#    mv "$appimage_name" "$REPO_ROOT/"
#    echo "✓ Created: $REPO_ROOT/$appimage_name"
#  fi
#  if [ -f "$appimage_name.zsync" ]; then
#    mv "$appimage_name.zsync" "$REPO_ROOT/"
#    echo "✓ Created: $REPO_ROOT/$appimage_name.zsync"
#  fi
#}

#build_tarball() {
#  echo "=== Building Tarball ==="
#
#  local version="$(get_app_version)"
#  local tarball_name="${APP_NAME}_${PLATFORM_TAG}_v${version}.tar.gz"
#  local tar_dir="$TEMP_DIR/tarball/moonfin-${version}"
#
#  rm -rf "$TEMP_DIR/tarball"
#  mkdir -p "$tar_dir"
#  cp -r "$BUILD_DIR"/* "$tar_dir/"
#  inject_linux_runtime_libs "$tar_dir"
#
#  if [ -f "$tar_dir/moonfin" ]; then
#    mv "$tar_dir/moonfin" "$tar_dir/moonfin-bin"
#    cat > "$tar_dir/moonfin" << 'EOF'
##!/bin/sh
#APPDIR="$(cd "$(dirname "$0")" && pwd)"
#export LD_LIBRARY_PATH="$APPDIR/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
#
## nix-ld only hands NixOS programs their libraries once they start, and ldd
## runs Nix's own loader, which never looks there, so the check adds them.
#check_lib_path="$LD_LIBRARY_PATH${NIX_LD_LIBRARY_PATH:+:$NIX_LD_LIBRARY_PATH}"
#
#resolve_exact_lib() {
#  lib_name="$1"
#  old_ifs="$IFS"
#  IFS=':'
#  for lib_dir in $check_lib_path; do
#    [ -n "$lib_dir" ] && [ -e "$lib_dir/$lib_name" ] && IFS="$old_ifs" && return 0
#  done
#  IFS="$old_ifs"
#
#  for lib_dir in /lib /lib64 /usr/lib /usr/lib64 /usr/lib/x86_64-linux-gnu /usr/lib/aarch64-linux-gnu; do
#    [ -e "$lib_dir/$lib_name" ] && return 0
#  done
#
#  return 1
#}
#
#ldd_output="$(LD_LIBRARY_PATH="$check_lib_path" ldd "$APPDIR/moonfin-bin" 2>/dev/null || true)"
#
## Only "libfoo.so => not found" lines name a missing library. Symbol version
## lines start with the binary path, so matching bare "not found" reports the
## binary itself as missing.
#missing_libs="$(printf '%s\n' "$ldd_output" | awk '$2 == "=>" && $3 == "not" && $4 == "found" {print $1}' | sort -u)"
#version_errors="$(printf '%s\n' "$ldd_output" | grep -F 'not found' | grep -v ' => ' || true)"
#
#if ! resolve_exact_lib libsqlite3.so; then
#  missing_libs="$(printf '%s\n%s\n' "$missing_libs" "libsqlite3.so" | awk 'NF' | sort -u)"
#fi
#
#if [ -n "$missing_libs" ]; then
#  echo "Moonfin cannot start. Missing shared libraries:" >&2
#  printf '  - %s\n' $missing_libs >&2
#  echo "Install the missing libraries for your distro and retry." >&2
#  exit 127
#fi
#
#if [ -n "$version_errors" ]; then
#  echo "Moonfin cannot start. A library your system provides is missing symbols" >&2
#  echo "this build needs:" >&2
#  printf '%s\n' "$version_errors" >&2
#  echo "That library has to be bundled with the app for your distro." >&2
#  echo "Please report this output so the next build carries it." >&2
#  echo "A deb, rpm, flatpak or snap build works in the meantime." >&2
#  exit 127
#fi
#
#exec "$APPDIR/moonfin-bin" "$@"
#EOF
#    chmod +x "$tar_dir/moonfin"
#  fi
#
#  cat > "$tar_dir/README.txt" << EOF
#Moonfin ${version}
#Jellyfin & Emby media client for Linux
#
#Installation:
#  1. Extract this archive
#  2. Run ./moonfin
#
#Dependencies:
#  - GTK 3.0+
#  - GLib 2.0+
#  - WebKitGTK 4.1 runtime (libwebkit2gtk-4.1-0)
#  - libflutter_linux_gtk (bundled)
#  - Additional runtime libs are bundled when available (libmpv, libsecret)
#  - xdg-user-dirs
#
#Requirements:
#  - X11 or Wayland display server
#  - Network access to Jellyfin/Emby server
#EOF
#
#  # The AUR package is built from this tarball, so it has to carry the desktop
#  # entry and the icon. Without them there is nothing for a packager to install
#  # and the window falls back to a placeholder.
#  create_desktop_file "$tar_dir/share/applications"
#  create_metainfo_file "$tar_dir/share/metainfo" "$version"
#  install_icons "$tar_dir/share"
#
#  cd "$TEMP_DIR/tarball"
#  tar -czf "$tarball_name" "moonfin-${version}"
#  mv "$tarball_name" "$REPO_ROOT/"
#  echo "✓ Created: $REPO_ROOT/$tarball_name"
#}

# The newest glibc anything in the installed tree asks for, which is the oldest
# glibc that can load it, since symbol versions are forward compatible only.
# Only undefined symbols count, because a library that defines a version lists
# every version it ever published rather than the one it needs.
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

#build_deb() {
#  echo "=== Building Debian Package (.deb) ==="
#  if ! command -v dpkg-deb >/dev/null 2>&1; then
#    echo "Skipping .deb: dpkg-deb not found"
#    return 1
#  fi
#
#  local version="$(get_app_version)"
#  local deb_name="${APP_NAME}_${PLATFORM_TAG}_v${version}.deb"
#  local pkg_root="$TEMP_DIR/deb/moonfin-${version}"
#  local deb_arch
#  deb_arch="$(get_deb_architecture)"
#
#  rm -rf "$TEMP_DIR/deb"
#  mkdir -p "$pkg_root"/{usr/bin,usr/lib/moonfin,usr/share/applications,usr/share/pixmaps,usr/share/doc/moonfin,DEBIAN}
#
#  cp -r "$BUILD_DIR"/* "$pkg_root/usr/lib/moonfin/"
#
#  inject_linux_runtime_libs "$pkg_root/usr/lib/moonfin"
#
#  cat > "$pkg_root/usr/bin/moonfin" << 'EOF'
##!/bin/sh
#APPDIR="/usr/lib/moonfin"
#export LD_LIBRARY_PATH="$APPDIR/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
#exec "$APPDIR/moonfin" "$@"
#EOF
#  chmod +x "$pkg_root/usr/bin/moonfin"
#
#  create_desktop_file "$pkg_root/usr/share/applications"
#  install_icons "$pkg_root/usr/share"
#
#  mkdir -p "$pkg_root/usr/share/metainfo"
#  create_metainfo_file "$pkg_root/usr/share/metainfo" "$version"
#
#  # glibc is deliberately not bundled, so the build host's version is the oldest
#  # one the package can run on. Declaring it turns a wall of loader errors at
#  # launch into a refused install that names the glibc the machine needs.
#  local depends="libgtk-3-0, libglib2.0-0, libsecret-1-0, libwebkit2gtk-4.1-0, xdg-user-dirs"
#  local glibc
#  glibc="$(required_glibc_version "$pkg_root/usr/lib/moonfin")"
#  if [ -n "$glibc" ]; then
#    depends="$depends, libc6 (>= $glibc)"
#  fi
#  echo "Depends: $depends"
#
#  cat > "$pkg_root/DEBIAN/control" << EOF
#Package: moonfin
#Version: ${version}
#Architecture: ${deb_arch}
#Maintainer: Moonfin Team <support@moonfin.dev>
#Installed-Size: $(du -sk "$pkg_root/usr" | cut -f1)
#Depends: ${depends}
#Description: Jellyfin & Emby media client
# Moonfin is a media client for Jellyfin and Emby servers,
# available on mobile, TV, and desktop platforms.
# .
# Features:
#  - Browse and stream media
#  - Offline downloads
#  - Casting support
#  - DLNA playback
#Homepage: https://moonfin.app/
#License: GPL-3.0-only
#EOF
#
#  cat > "$pkg_root/usr/share/doc/moonfin/copyright" << EOF
#Format: https://www.debian.org/doc/packaging-manuals/copyright-format/1.0/
#Upstream-Name: Moonfin
#Upstream-Contact: https://github.com/jmshrv/Moonfin
#Source: https://github.com/jmshrv/Moonfin
#
#Files: *
#Copyright: 2024-2025 Moonfin Team
#License: GPL-3.0-only
#EOF
#
#  cd "$TEMP_DIR/deb"
#  dpkg-deb --build "moonfin-${version}" "$deb_name" 2>/dev/null || true
#
#  if [ -f "$deb_name" ]; then
#    mv "$deb_name" "$REPO_ROOT/"
#    echo "✓ Created: $REPO_ROOT/$deb_name"
#  fi
#}

#build_rpm() {
#  echo "=== Building RPM Package (.rpm) ==="
#  if ! command -v rpmbuild >/dev/null 2>&1; then
#    echo "Skipping .rpm: rpmbuild not found"
#    return 1
#  fi
#
#  local version="$(get_app_version)"
#  local rpm_name="${APP_NAME}_${PLATFORM_TAG}_v${version}.rpm"
#  local rpm_dir="$TEMP_DIR/rpm"
#  local spec_file="$rpm_dir/moonfin.spec"
#
#  rm -rf "$rpm_dir"
#  mkdir -p "$rpm_dir"/{SPECS,SOURCES,BUILD,RPMS,SRPMS}
#  create_desktop_file "$rpm_dir"
#  create_metainfo_file "$rpm_dir" "$version"
#
#  cat > "$spec_file" << EOF
#Name:           moonfin
#Version:        ${version}
#Release:        1
#Summary:        Jellyfin & Emby media client
#License:        GPL-3.0-only
#Requires:       xdg-user-dirs
#
#%description
#Moonfin is a media client for Jellyfin and Emby servers,
#available on mobile, TV, and desktop platforms.
#
#%install
#mkdir -p %{buildroot}/usr/bin
#mkdir -p %{buildroot}/usr/lib/moonfin
#mkdir -p %{buildroot}/usr/share/applications
#mkdir -p %{buildroot}/usr/share/pixmaps
#mkdir -p %{buildroot}/usr/share/metainfo
#
#cp -r ${BUILD_DIR}/* %{buildroot}/usr/lib/moonfin/
#
#cat > %{buildroot}/usr/bin/moonfin << 'EOFRUNNER'
##!/bin/sh
#APPDIR="/usr/lib/moonfin"
#export LD_LIBRARY_PATH="\$APPDIR/lib\${LD_LIBRARY_PATH:+:\$LD_LIBRARY_PATH}"
#exec "\$APPDIR/moonfin" "\$@"
#EOFRUNNER
#chmod +x %{buildroot}/usr/bin/moonfin
#
#cp "$rpm_dir/${APP_ID}.desktop" %{buildroot}/usr/share/applications/${APP_ID}.desktop
#cp "$rpm_dir/${APP_ID}.metainfo.xml" %{buildroot}/usr/share/metainfo/${APP_ID}.metainfo.xml
#
#if [ -f "$APP_ICON" ]; then
#  mkdir -p %{buildroot}/usr/share/icons/hicolor/512x512/apps
#  cp "$APP_ICON" %{buildroot}/usr/share/icons/hicolor/512x512/apps/${APP_ID}.png
#  cp "$APP_ICON" %{buildroot}/usr/share/pixmaps/${APP_ID}.png
#fi
#
#%files
#/usr/bin/moonfin
#%dir /usr/lib/moonfin
#/usr/lib/moonfin/*
#/usr/share/applications/${APP_ID}.desktop
#/usr/share/icons/hicolor/512x512/apps/${APP_ID}.png
#/usr/share/pixmaps/${APP_ID}.png
#/usr/share/metainfo/${APP_ID}.metainfo.xml
#
#%changelog
#* $(date '+%a %b %d %Y') Moonfin Team <support@moonfin.dev>
#- Release ${version}
#EOF
#
#  cd "$rpm_dir"
#  rpmbuild --define "_topdir $rpm_dir" -bb "$spec_file" || true
#  local rpm_file=$(find "$rpm_dir/RPMS" -name "*.rpm" 2>/dev/null | head -1)
#  if [ -n "$rpm_file" ]; then
#    cp "$rpm_file" "$REPO_ROOT/$rpm_name"
#    echo "✓ Created: $REPO_ROOT/$rpm_name"
#  fi
#}

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

#build_snap() {
#  echo "=== Building Snap Package ==="
#  if ! command -v snapcraft >/dev/null 2>&1; then
#    echo "Skipping Snap: snapcraft not found"
#    echo "  Install: sudo snap install snapcraft --classic"
#    return 1
#  fi
#
#  local snap_base
#  if ! snap_base="$(get_snap_base)"; then
#    echo "Skipping Snap: no base snap matches this host"
#    echo "  Staged packages don't run on a base built from another release."
#    return 1
#  fi
#
#  if command -v snap >/dev/null 2>&1; then
#    echo "Waiting for snapd seed to complete..."
#    if ! snap wait system seed.loaded >/dev/null 2>&1; then
#      echo "Warning: snapd seed wait failed; continuing and letting snapcraft attempt build."
#    fi
#
#    if ! snap list "$snap_base" >/dev/null 2>&1; then
#      echo "$snap_base not found. Installing $snap_base base snap..."
#      if ! retry_with_backoff 3 sudo snap install "$snap_base" --channel=latest/stable; then
#        echo "Failed to install $snap_base after retries."
#        return 1
#      fi
#    fi
#  fi
#
#  local snap_dir="$TEMP_DIR/snap"
#  local version="$(get_app_version)"
#  local snap_mpv_package
#  snap_mpv_package="$(resolve_mpv_package_name)"
#  local snap_triplet
#  snap_triplet="$(get_multiarch_triplet)"
#
#  rm -rf "$snap_dir"
#  mkdir -p "$snap_dir"
#
#  cat > "$snap_dir/snapcraft.yaml" << EOF
#name: moonfin
#title: Moonfin
#version: '${version}'
#summary: Jellyfin & Emby media client
#description: |
#  Moonfin is a media client for Jellyfin and Emby servers.
#  Stream movies, TV shows, music, and photos from your server.
#
#grade: stable
#confinement: strict
#base: ${snap_base}
#icon: ${APP_ID}.png
#
#apps:
#  moonfin:
#    command: moonfin
#    plugs:
#      - home
#      - network
#      - network-bind
#      - opengl
#      - pulseaudio
#    environment:
#      # libpulse looks for libpulsecommon under an absolute path that
#      # confinement points at the base snap, which carries no audio libraries,
#      # so the copy packed in here has to be named on the search path.
#      LD_LIBRARY_PATH: \$SNAP/lib:\$SNAP/lib/${snap_triplet}:\$SNAP/usr/lib/${snap_triplet}:\$SNAP/usr/lib/${snap_triplet}/pulseaudio
#
#parts:
#  moonfin:
#    plugin: dump
#    source: .
#    override-build: |
#      mkdir -p \$SNAPCRAFT_PART_INSTALL/bin
#      mkdir -p \$SNAPCRAFT_PART_INSTALL/lib
#      cp -r ${BUILD_DIR}/* \$SNAPCRAFT_PART_INSTALL/
#    stage-packages:
#      - libgtk-3-0
#      - libglib2.0-0
#      - libx11-6
#      - ${snap_mpv_package}
#      - libsecret-1-0
#      - libwebkit2gtk-4.1-0
#EOF
#
#  cp -r "$BUILD_DIR"/* "$snap_dir/"
#  copy_icon "$snap_dir"
#
#  # snapcraft installs snap/gui into meta/gui and snapd rewrites the entry on
#  # install, which is the only way a strictly confined snap gets an icon.
#  mkdir -p "$snap_dir/snap/gui"
#  cat > "$snap_dir/snap/gui/moonfin.desktop" << EOF
#[Desktop Entry]
#Type=Application
#Name=Moonfin
#Exec=moonfin %U
#Icon=\${SNAP}/meta/gui/icon.png
#Categories=AudioVideo;Video;
#Comment=Jellyfin & Emby media client
#Terminal=false
#MimeType=x-scheme-handler/moonfin;
#EOF
#
#  cd "$snap_dir"
#  if ! retry_with_backoff 3 snapcraft pack --destructive-mode; then
#    echo "Snap build failed after retries"
#    return 1
#  fi
#
#  local snap_file
#  snap_file=$(find "$snap_dir" -maxdepth 1 -name "*.snap" 2>/dev/null | head -1)
#  if [ -n "$snap_file" ]; then
#    mv "$snap_file" "$REPO_ROOT/${APP_NAME}_${PLATFORM_TAG}_v${version}.snap"
#    echo "✓ Created: $REPO_ROOT/${APP_NAME}_${PLATFORM_TAG}_v${version}.snap"
#  else
#    echo "Snap build did not produce a .snap file"
#    return 1
#  fi
#}

#build_flatpak() {
#  echo "=== Building Flatpak Package ==="
#  if ! command -v flatpak-builder >/dev/null 2>&1; then
#    echo "Skipping Flatpak: flatpak-builder not found"
#    echo "  Install: sudo apt install flatpak-builder"
#    return 1
#  fi
#
#  local flatpak_dir="$TEMP_DIR/flatpak"
#  local version="$(get_app_version)"
#
#  rm -rf "$flatpak_dir"
#  mkdir -p "$flatpak_dir"
#
#  local flatpak_build_dir="$TEMP_DIR/flatpak-build"
#  local flatpak_repo_dir="$TEMP_DIR/flatpak-repo"
#  local flatpak_name="${APP_NAME}_${PLATFORM_TAG}_v${version}.flatpak"
#  local flatpak_src="$flatpak_dir/src"
#
#  rm -rf "$flatpak_build_dir" "$flatpak_repo_dir"
#  mkdir -p "$flatpak_src"
#  cp -r "$BUILD_DIR"/* "$flatpak_src/"
#  inject_flatpak_libs "$flatpak_src/lib/moonfin"
#  [ -f "$APP_ICON" ] && cp "$APP_ICON" "$flatpak_src/${APP_ID}.png"
#  create_desktop_file "$flatpak_src"
#  create_metainfo_file "$flatpak_src" "$version"
#
#  cat > "$flatpak_dir/${APP_ID}.yml" << EOF
#app-id: ${APP_ID}
#runtime: org.gnome.Platform
#runtime-version: '50'
#sdk: org.gnome.Sdk
#
#command: moonfin
#
#finish-args:
#  - --share=network
#  - --share=ipc
#  - --socket=fallback-x11
#  - --socket=wayland
#  # GPU and gamepads. --device=input is narrower, but the Ubuntu flatpak-builder
#  # in CI is too old to build it.
#  - --device=all
#  - --socket=pulseaudio
#  # MPRIS media controls
#  - --own-name=org.mpris.MediaPlayer2.moonfin
#  # Saved logins (flutter_secure_storage via libsecret)
#  - --talk-name=org.freedesktop.secrets
#  # Keep the screen awake during playback (wakelock_plus)
#  - --talk-name=org.freedesktop.ScreenSaver
#  # Desktop notifications (flutter_local_notifications)
#  - --talk-name=org.freedesktop.Notifications
#  # Online/offline detection (connectivity_plus)
#  - --system-talk-name=org.freedesktop.NetworkManager
#  # Earlier builds kept the offline database here. It's read once to copy it
#  # into the app's own data folder.
#  - --filesystem=xdg-documents/Moonfin:ro
#
#modules:
#  - name: appstream-compose-shim
#    # Shim for SDKs that dropped appstream-compose (24.08+); appstreamcli compose replaces it.
#    buildsystem: simple
#    build-commands:
#      - mkdir -p /app/bin
#      - cp appstream-compose /app/bin/appstream-compose
#      - chmod +x /app/bin/appstream-compose
#    sources:
#      - type: script
#        dest-filename: appstream-compose
#        commands:
#          - |
#            #!/usr/bin/env bash
#            args=()
#            basename_val=""
#            skip_next=0
#            for arg in "\$@"; do
#              if [[ "\$skip_next" -eq 1 ]]; then
#                basename_val="\$arg"
#                skip_next=0
#                continue
#              fi
#              if [[ "\$arg" == --basename ]]; then
#                skip_next=1
#                continue
#              fi
#              if [[ "\$arg" == --basename=* ]]; then
#                basename_val="\${arg#--basename=}"
#                continue
#              fi
#              if [[ -n "\$basename_val" && "\$arg" == "\$basename_val" ]]; then
#                args+=("/")
#                continue
#              fi
#              args+=("\$arg")
#            done
#            exec appstreamcli compose "\${args[@]}"
#  - name: moonfin
#    buildsystem: simple
#    build-commands:
#      - mkdir -p /app/bin /app/moonfin /app/share/applications /app/share/metainfo /app/share/icons/hicolor/512x512/apps
#      - cp -r . /app/moonfin/
#      - chmod +x /app/moonfin/moonfin
#      - |
#        cat > /app/bin/moonfin << 'EOFRUN'
#        #!/bin/sh
#        export LD_LIBRARY_PATH="/app/moonfin/lib/moonfin:/app/moonfin/lib\${LD_LIBRARY_PATH:+:\$LD_LIBRARY_PATH}"
#
#        resolve_exact_lib() {
#          lib_name="\$1"
#          old_ifs="\$IFS"
#          IFS=':'
#          for lib_dir in \$LD_LIBRARY_PATH; do
#            [ -n "\$lib_dir" ] && [ -e "\$lib_dir/\$lib_name" ] && IFS="\$old_ifs" && return 0
#          done
#          IFS="\$old_ifs"
#
#          for lib_dir in /lib /lib64 /usr/lib /usr/lib64 /usr/lib/x86_64-linux-gnu /usr/lib/aarch64-linux-gnu; do
#            [ -e "\$lib_dir/\$lib_name" ] && return 0
#          done
#
#          return 1
#        }
#
#        ldd_output="\$(ldd /app/moonfin/moonfin 2>/dev/null || true)"
#
#        # Only "libfoo.so => not found" lines name a missing library. Symbol
#        # version lines start with the binary path, so matching bare "not found"
#        # reports the binary itself as missing.
#        missing_libs="\$(printf '%s\\n' "\$ldd_output" | awk '\$2 == "=>" && \$3 == "not" && \$4 == "found" {print \$1}' | sort -u)"
#        version_errors="\$(printf '%s\\n' "\$ldd_output" | grep -F 'not found' | grep -v ' => ' || true)"
#
#        if ! resolve_exact_lib libsqlite3.so; then
#          missing_libs="\$(printf '%s\\n%s\\n' "\$missing_libs" "libsqlite3.so" | awk 'NF' | sort -u)"
#        fi
#
#        if [ -n "\$missing_libs" ]; then
#          echo "Moonfin cannot start. Missing shared libraries:" >&2
#          printf '  - %s\\n' \$missing_libs >&2
#          echo "Install the missing libraries in the runtime and retry." >&2
#          exit 127
#        fi
#
#        if [ -n "\$version_errors" ]; then
#          echo "Moonfin cannot start. A library the Flatpak runtime provides is missing" >&2
#          echo "symbols this build needs:" >&2
#          printf '%s\\n' "\$version_errors" >&2
#          echo "The bundled libraries were built against a newer system than the runtime." >&2
#          echo "Please report this output." >&2
#          exit 127
#        fi
#
#        exec /app/moonfin/moonfin "\$@"
#        EOFRUN
#      - chmod +x /app/bin/moonfin
#      - '[ -f ${APP_ID}.png ] && cp ${APP_ID}.png /app/share/icons/hicolor/512x512/apps/ || true'
#      - cp ${APP_ID}.desktop /app/share/applications/
#      - cp ${APP_ID}.metainfo.xml /app/share/metainfo/
#    sources:
#      - type: dir
#        path: src
#EOF
#
#  mkdir -p "$flatpak_repo_dir"
#  ostree init --repo="$flatpak_repo_dir" --mode=archive-z2 2>/dev/null || true
#  ostree --repo="$flatpak_repo_dir" config set 'core.min-free-space-percent' 0 2>/dev/null || true
#
#  ensure_flatpak_runtime
#
#  flatpak-builder --force-clean \
#    --repo="$flatpak_repo_dir" \
#    "$flatpak_build_dir" \
#    "$flatpak_dir/${APP_ID}.yml"
#
#  if flatpak build-bundle "$flatpak_repo_dir" "$REPO_ROOT/$flatpak_name" "${APP_ID}"; then
#    echo "✓ Created: $REPO_ROOT/$flatpak_name"
#  else
#    echo "Flatpak build did not produce a bundle"
#  fi
#}

main() {
#  local formats="${1:-all}"
#  formats="$(printf '%s\n' "$formats" | tr '[:upper:]' '[:lower:]')"
  local version="$(get_app_version)"

  echo "======================================"
  echo "Moonfin Linux Package Builder"
  echo "Version: ${version}"
  echo "======================================"
  echo ""

  #build_flutter_binary
  BUILD_DIR="$(resolve_build_dir)"
  inject_flatpak_libs "/lib/moonfin"
  if [ "${BUNDLE_RUNTIME_LIBS:-0}" = "1" ]; then
    inject_linux_runtime_libs "$BUILD_DIR"
  else
    echo "Skipping bundled runtime libs (set BUNDLE_RUNTIME_LIBS=1 to enable)."
  fi
  rm -rf "$TEMP_DIR"

#  case "$formats" in
#    all)
#      build_tarball || true
#      build_appimage || true
#      build_deb || true
#      build_rpm || true
#      build_snap || true
#      build_flatpak || true
#      ;;
#    tarball)
#      build_tarball
#      ;;
#    appimage)
#      build_appimage
#      ;;
#    deb)
#      build_deb
#      ;;
#    rpm)
#      build_rpm
#      ;;
#    snap)
#      build_snap
#      ;;
#    flatpak)
#      build_flatpak
#      ;;
#    *)
#      cat << USAGE
#Usage: $0 [FORMAT]
#
#Available formats:
#  all          Build all package formats (default)
#  tarball      Create tarball (.tar.gz)
#  appimage     Create AppImage (requires appimagetool)
#  deb          Create Debian package (requires dpkg-deb)
#  rpm          Create RPM package (requires rpmbuild)
#  snap         Create Snap package (requires snapcraft)
#  flatpak      Create Flatpak package (requires flatpak-builder)

#Examples:
#  $0                 # Build all formats
#  $0 tarball         # Build only tarball
#  $0 appimage        # Build only AppImage
#USAGE
#      exit 1
#      ;;
#  esac

#  rm -rf "$TEMP_DIR"

  echo ""
  echo "======================================"
  echo "Build complete!"
  echo "Artifacts: $REPO_ROOT"
  ls -lh "$REPO_ROOT"/Moonfin_* 2>/dev/null || echo "(no artifacts built)"
  echo "======================================"
}

main "$@"
