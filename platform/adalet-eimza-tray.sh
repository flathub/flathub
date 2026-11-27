#!/bin/sh
set -e

jar=$(ls /app/extra/eimza-tray-*.jar)
version=${jar##*/eimza-tray-}
version=${version%.jar}

# OpenJDK only looks for libpcsclite in /usr/lib64. Set through the environment so
# the card reader scan, which is a second Java process, inherits it.
export JAVA_TOOL_OPTIONS="-Dsun.security.smartcardio.library=/app/lib/libpcsclite.so.1"

# the tray icon is written to the temp dir and read by the desktop, which cannot
# see the sandbox's /tmp
tmp=$XDG_CACHE_HOME/tmp
mkdir -p "$tmp"

# apart from the temp dir, same options as upstream's launcher
run_java() {
    exec /app/jre/bin/java \
        -Djava.io.tmpdir="$tmp" \
        --add-modules=java.logging,java.management,java.desktop,java.security.jgss,jdk.crypto.cryptoki \
        --add-exports=jdk.crypto.cryptoki/sun.security.pkcs11.wrapper=ALL-UNNAMED \
        --add-opens=java.base/java.lang=ALL-UNNAMED \
        --add-opens=java.base/java.util=ALL-UNNAMED \
        --add-opens=java.smartcardio/sun.security.smartcardio=ALL-UNNAMED \
        -Xmx512m \
        -Dfile.encoding=UTF-8 \
        -Djava.net.useSystemProxies=true \
        -Dapp.dir=/app/extra \
        -Dapp.displayName="Adalet Eimza" \
        -Dapp.fsname=adalet-eimza-tray \
        -Dapp.version="$version" \
        -cp "$jar" org.springframework.boot.loader.launch.JarLauncher "$@"
}

. /app/libexec/dialogs.sh

# Shared by every instance: the one holding its lock is the running app,
# and a line written to it asks that app to quit.
pipe=$XDG_RUNTIME_DIR/app/$FLATPAK_ID/quit
[ -p "$pipe" ] || mkfifo "$pipe"
exec 9<>"$pipe"

if ! flock -n 9; then
    # Already running. Started again, upstream finds its port taken and shows
    # a "running in the background" notice, which is enough when there is a tray.
    if has_tray; then
        run_java "$@"
    fi
    if ask_quit; then
        echo quit >&9
    fi
    exit
fi

run_java "$@" &
java=$!

(read -r _ <&9; kill "$java") &
watcher=$!

has_reader_service || warn_reader &
has_tray || { ask_quit && echo quit >&9; } &

status=0
wait "$java" || status=$?

# close the watcher and any notice still open
kill "$watcher" 2>/dev/null || true
pkill -x zenity || true
exit "$status"
