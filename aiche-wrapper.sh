#!/bin/bash
# AICHE Desktop launcher for the Flathub build.
#
# The application code ships as extra-data. apply_extra extracts the payload to
# /app/extra/aiche-desktop on first run, so that is where the code lives here
# (the website build keeps it under /app/lib instead).

APPLIB="/app/extra/aiche-desktop"

PYVER=$(python3 -c "import sys; print(f'{sys.version_info.major}.{sys.version_info.minor}')")

export PYTHONPATH="${APPLIB}:/app/lib/python${PYVER}/site-packages:/usr/lib/python3/dist-packages:/usr/lib/python${PYVER}/site-packages"
export PYTHONNOUSERSITE=1

# ten_vad's libten_vad.so needs libc++, which the KDE runtime does not ship. The
# payload bundles the same libs the DEB carries; put them ahead on the loader path.
export LD_LIBRARY_PATH="${APPLIB}/lib/compat:${LD_LIBRARY_PATH}"

if [ "$XDG_SESSION_TYPE" = "wayland" ] || [ -n "$WAYLAND_DISPLAY" ]; then
    export QT_QPA_PLATFORM=wayland
else
    export QT_QPA_PLATFORM=xcb
fi

# ffmpeg binary from the org.freedesktop.Platform.ffmpeg-full extension.
export PATH="/app/lib/ffmpeg/bin:${PATH}"

export AICHE_FLATPAK=1
export AICHE_PRODUCTION_MODE="${AICHE_PRODUCTION_MODE:-true}"
export QT_LOGGING_RULES="*=false"

cd "${APPLIB}"
exec python3 -m src.main "$@"
