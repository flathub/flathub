#!/bin/bash

for i in {0..9}; do
	test -S $XDG_RUNTIME_DIR/discord-ipc-$i || ln -sf {app/com.discordapp.Discord,$XDG_RUNTIME_DIR}/discord-ipc-$i;
done

if [[ -z $(find ~/.srb2kart -name bonuschars.kart) ]]; then
	ln -s /app/extra/bonuschars.kart ~/.srb2kart/bonuschars.kart
fi

export PATH="/app/utils/gamescope/bin:$PATH"

if [[ -z "$SRB2WADDIR" ]]; then
	export SRB2WADDIR=/app/extra
fi

if [ -z "$GAMESCOPE_ARGS" ]; then
	GAMESCOPE_ARGS="-h 1080 -C 1000 -i"
fi

if [[ -n $(command -v gamescope) && -n "$GAMESCOPE_ENABLE" ]]; then
	echo "$GAMESCOPE_ARGS" -- srb2kart "$@" | xargs gamescope
else
	srb2kart "$@"
fi
