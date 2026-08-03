#!/bin/sh
# Startskript för Uni Personal i Flatpak.
set -e

# Skarpt läge lägger nyttolasten i /app/extra (dit extra-data packas upp).
# /app/extra är reserverat av flatpak och rensas bort vid installation om
# manifestet saknar extra-data — därför lägger local-test.yml den i
# /app/unipersonal i stället. Skriptet väljer den som finns, så exakt samma
# startskript provkörs som det som skickas till Flathub.
APPDIR=/app/extra
[ -d "$APPDIR" ] || APPDIR=/app/unipersonal

export LD_LIBRARY_PATH="$APPDIR${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

# Klippbordsvakten startas normalt via /etc/xdg/autostart. En flatpak får inte
# skriva där, så appen startar den själv — villkorslöst. Ingen pidfil: varje
# `flatpak run` får en egen PID-namnrymd men delar XDG_RUNTIME_DIR, så en
# sparad PID kan råka peka på en levande process i en annan instans. Vakten
# tar i stället ett exklusivt lås på historikfilen och avslutar sig själv om
# en annan redan kör.
if [ -x "$APPDIR/unipersonal-clipboard" ]; then
    "$APPDIR/unipersonal-clipboard" >/dev/null 2>&1 &
fi

cd "$APPDIR"
exec "$APPDIR/UniPersonal.Desktop" "$@"
