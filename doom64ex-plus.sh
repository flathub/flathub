#!/usr/bin/bash

# Setup data folder path
game_data_dir=${XDG_DATA_HOME}/doom64ex-plus

# Check data folder
if [ ! -d ${game_data_dir} ]; then
    zenity --width=400 --error --title="Game data files not found" \
    --no-wrap --text="Game data files must be copied to:\n${game_data_dir}" \
    --ok-label "Close"
    exit 1
fi

# Copy support data files
cp -u /app/share/games/doom64ex-plus/*.* ${game_data_dir}

# Run game engine
exec DOOM64EX-Plus "$@"
