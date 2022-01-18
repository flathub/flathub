#!/bin/sh

if [ ! -f "$XDG_CONFIG_HOME/first_start" ]; then
    echo "First start: running kbuildsycoca5"
    touch $XDG_CONFIG_HOME/first_start
    # Dolphin requires that ksycoca cache exists, but cannot run kbuildsycoca5
    # automatically (because KDED lives outside of the sandbox).
    # As a workaround we force-run it ourselves. It's really only needed once.
    kbuildsycoca5
fi

exec dolphin-bin "$@"
