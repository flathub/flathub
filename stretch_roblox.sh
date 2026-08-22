#!/bin/bash
# 1. Talk to host system to turn ON stretch scaling
flatpak-spawn --host xrandr --output eDP --set "scaling mode" "Full"
flatpak-spawn --host xrandr --output eDP --mode 1024x768

# 2. Launch the standard Sober flatpak on the host system
flatpak-spawn --host flatpak run org.vinegarhq.Sober

# 3. Talk to host system to turn OFF stretch scaling on exit
flatpak-spawn --host xrandr --output eDP --set "scaling mode" "None"
flatpak-spawn --host xrandr --output eDP --mode 1024x768
