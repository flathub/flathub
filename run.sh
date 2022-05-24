#!/bin/sh

GAME_PATH="$XDG_DATA_HOME/Duke2"

if ls "$GAME_PATH/NUKEM2.CMP" > /dev/null 2>&1; then
    RigelEngine "$@" "$GAME_PATH"
else
    zenity --error --text "Missing data - place your Duke Nukem II game data into:\n$GAME_PATH" --ok-label "Close" --width=500
fi
