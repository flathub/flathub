#!/bin/sh
set -eu

# Runs offline at install time inside a minimal Flatpak sandbox. Assembles IBKR
# Desktop from the pre-downloaded extra-data files (installer + JRE + internal
# jars) by running the install4j installer headlessly, then strips anything that
# would let install4j manage updates or remember build-time paths.

extra_root="${EXTRA_ROOT:-/app/extra}"
cd "$extra_root"

installer=ntws-installer.sh
jre_archive=zulu-jre.tar.gz
installer_jre_archive=jre.tar.gz
install_dir="$extra_root/ibkr"

# Scratch dirs: the installer's writes (home, XDG, caches) are redirected here
# and removed at the end, so nothing leaks into the finished extension.
jre_stage="$extra_root/.jre-stage"
fake_home="$extra_root/.fake-home"
fake_data="$extra_root/.fake-data"
fake_config="$extra_root/.fake-config"
fake_cache="$extra_root/.fake-cache"
fake_bin="$extra_root/.fake-bin"

# --- helpers ---

require_file() {
    if [ ! -f "$1" ]; then
        echo "missing required extra-data file: $1" >&2
        exit 1
    fi
}

jre_mtime() {
    date -r "$1" "+%s" 2>/dev/null && return
    stat -c "%Y" "$1" 2>/dev/null && return
    echo 0
}

# --- locate inputs ---

require_file "$installer"
require_file "$jre_archive"

# The internal jars carry a build timestamp in their name; fall back to the
# untimestamped name in case update-sources.sh ever changes the convention.
launch_jar="$(find . -maxdepth 1 -type f -name 'ntws4launch-*-linux-x64_[0-9]*.jar' -printf '%f\n' | head -n 1)"
if [ -z "$launch_jar" ]; then
    launch_jar="$(find . -maxdepth 1 -type f -name 'ntws4launch-*-linux-x64.jar' -printf '%f\n' | head -n 1)"
fi

qml_jar="$(find . -maxdepth 1 -type f -name 'ntws4qml-*_[0-9]*.jar' -printf '%f\n' | head -n 1)"
if [ -z "$qml_jar" ]; then
    qml_jar="$(find . -maxdepth 1 -type f -name 'ntws4qml-*.jar' -printf '%f\n' | head -n 1)"
fi

require_file "$launch_jar"
require_file "$qml_jar"

# --- prepare scratch dirs and JRE ---

rm -rf "$install_dir" "$jre_stage" "$fake_home" "$fake_data" "$fake_config" "$fake_cache" "$fake_bin"
rm -f "$installer_jre_archive"
mkdir -p "$jre_stage" "$fake_home/Desktop" "$fake_data" "$fake_config/fontconfig" "$fake_cache/fontconfig" "$fake_bin"

tar -xzf "$jre_archive" -C "$jre_stage"
jre_java="$(find "$jre_stage" -path '*/bin/java' -type f | head -n 1)"
if [ -z "$jre_java" ]; then
    echo "failed to find java in $jre_archive" >&2
    exit 1
fi
jre_home="$(dirname "$(dirname "$jre_java")")"

# install4j launches java via the system loader; point it at the right ld-linux
# and the bundled JRE libs so the installer's java runs inside the sandbox.
java_loader=
for candidate in /lib64/ld-linux-x86-64.so.2 /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2; do
    if [ -x "$candidate" ]; then
        java_loader="$candidate"
        break
    fi
done

if [ -n "$java_loader" ]; then
    java_library_path="$jre_home/lib:$jre_home/lib/server"
    sed -i "s|^INSTALL4J_JAVA_PREFIX=.*|INSTALL4J_JAVA_PREFIX=\"$java_loader --library-path $java_library_path\"|" "$installer"
fi

# Pre-seed install4j's JRE cache so it accepts the bundled JRE instead of
# scanning the system (which has none in the sandbox).
mkdir -p "$fake_cache/install4j"
java_mtime="$(jre_mtime "$jre_java")"
cat > "$fake_cache/install4j/jre_version" <<EOF
JRE_VERSION	$jre_home	17	0	10	0
JRE_INFO	$jre_home	1	$java_mtime	1
EOF

# The installer is a Java GUI app; give fontconfig a writable cache so it does
# not fail trying to write to read-only locations.
cat > "$fake_config/fontconfig/fonts.conf" <<EOF
<?xml version="1.0"?>
<!DOCTYPE fontconfig SYSTEM "urn:fontconfig:fonts.dtd">
<fontconfig>
  <dir>/usr/share/fonts</dir>
  <cachedir>$fake_cache/fontconfig</cachedir>
</fontconfig>
EOF

# install4j checks free space with `df -k` before installing; the sandbox's
# overlay/fuse mounts can make df emit non-numeric output and trip a false
# "not enough space" failure. This shim normalizes that one query.
cat > "$fake_bin/df" <<'EOF'
#!/bin/sh
set -eu

real_df=/usr/bin/df
if [ ! -x "$real_df" ]; then
    real_df=/bin/df
fi

if [ "${1:-}" = "-k" ] && [ "$#" -ge 2 ]; then
    target=
    for arg in "$@"; do
        target="$arg"
    done

    df_line="$("$real_df" -Pk "$target" 2>/dev/null | sed -n '2p')" || df_line=
    set -- $df_line
    if [ "$#" -ge 4 ]; then
        blocks="$2"
        used="$3"
        available="$4"
        mount_point="${6:-$target}"
        case "$blocks$used$available" in
            *[!0-9]*)
                exec "$real_df" -k "$target"
                ;;
        esac

        printf 'Filesystem 1K-blocks Used Available Use%% Mounted on\n'
        printf 'flatpak-extra %s %s %s 0%% %s\n' "$blocks" "$used" "$available" "$mount_point"
        exit 0
    fi

    exec "$real_df" -k "$target"
fi

exec "$real_df" "$@"
EOF
chmod +x "$fake_bin/df"

# --- run the installer (isolated env, see scratch dirs above) ---

env \
    HOME="$fake_home" \
    XDG_DATA_HOME="$fake_data" \
    XDG_CONFIG_HOME="$fake_config" \
    XDG_CACHE_HOME="$fake_cache" \
    FONTCONFIG_FILE="$fake_config/fontconfig/fonts.conf" \
    INSTALL4J_JAVA_HOME_OVERRIDE="$jre_home" \
    INSTALL4J_ADD_VM_PARAMS="-Duser.home=$fake_home" \
    PATH="$fake_bin:${PATH:-/usr/bin:/bin}" \
    sh "$installer" -q -dir "$install_dir" -overwrite

# --- finalize the installed tree ---

# Bundle the JRE and the two internal jars (fetched as extra-data, not by the
# installer, since apply_extra has no network).
mkdir -p "$install_dir/jre"
cp -a "$jre_home"/. "$install_dir/jre"/

install -Dm644 "$launch_jar" "$install_dir/lib/$launch_jar"
install -Dm644 "$qml_jar" "$install_dir/lib/$qml_jar"

# The Flatpak is updated by Flathub, not by install4j at runtime.
sed -i \
    's#com.ib.tws.twslaunch.install4j.Install4jAutoUpdateService#twslaunch.autoupdate.DummyAutoUpdateService#g' \
    "$install_dir/ntws"

# The real install4j auto-update service also closes the Java splash screen.
# Since runtime updates are disabled above, prevent creating that splash.
sed -i \
    -e 's#"-splash:\$app_home/.install4j/[^"]*" ##g' \
    -e 's#"-DsplashScreenEnabled=true"#"-DsplashScreenEnabled=false"#g' \
    "$install_dir/ntws"

# The wrapper supplies the JRE via INSTALL4J_JAVA_HOME_OVERRIDE at runtime.
sed -i 's|^INSTALL4J_JAVA_PREFIX=.*|INSTALL4J_JAVA_PREFIX=|' "$install_dir/ntws"

# Avoid install4j remembering build-time paths.
: > "$install_dir/.install4j/inst_jre.cfg" || true
: > "$install_dir/.install4j/pref_jre.cfg" || true

if [ -f "$install_dir/.install4j/response.varfile" ]; then
    sed -i \
        -e 's|^appConfigDir=.*|appConfigDir=/var/data/ntws|' \
        -e 's|^createDesktopIcon.*|createDesktopIcon$Boolean=false|' \
        "$install_dir/.install4j/response.varfile"
fi

chmod +x "$install_dir/ntws"

# --- clean up scratch ---

rm -rf "$jre_stage" "$fake_home" "$fake_data" "$fake_config" "$fake_cache" "$fake_bin"
rm -f "$installer" "$jre_archive" "$installer_jre_archive" "$launch_jar" "$qml_jar"
