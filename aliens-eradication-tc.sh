#!/usr/bin/bash
#
# Aliens Eradication TC launcher script

if [ ! -f /var/config/uzdoom/uzdoom.ini ]; then
    echo "Copying default aliens-eradication-tc.ini to uzdoom.ini"
    mkdir -p /var/config/uzdoom
    cp /app/share/games/uzdoom/aliens-eradication-tc.ini /var/config/uzdoom/uzdoom.ini
fi

WM_CLASS="${FLATPAK_ID:-uzdoom}"

# Export environment variables for SDL2 (UZDoom)
export SDL_VIDEO_X11_WMCLASS="$WM_CLASS"
export SDL_VIDEO_WAYLAND_WMCLASS="$WM_CLASS"

# Run game engine
exec uzdoom -iwad freedoom2.wad \
    -file ERADICATION_MAPSET_2_0.wad \
    -file ALIENS_ERADICATION_TC_2_0.pk3 \
    "$@"
