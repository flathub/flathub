#!/usr/bin/bash

# Set game folder paths
rbdoom3bfg_data_dir=${XDG_DATA_HOME}/rbdoom3bfg
rbdoom3bfg_base_dir=${rbdoom3bfg_data_dir}/base

# Check data folder
if [ ! -d ${rbdoom3bfg_data_dir} ]
then
    mkdir -p ${rbdoom3bfg_data_dir}
fi

# Check base folder
if [ ! -d ${rbdoom3bfg_base_dir} ]
then
    zenity --width=400 --error --title="Game data files not found" \
    --no-wrap --text="The /base game folder must be copied to:\n${rbdoom3bfg_base_dir}" \
    --ok-label "Close"
    exit 1
elif [ ! -f ${rbdoom3bfg_data_dir}/.setup ]
then
    # Base folder exists, but it is not initialized
    echo "Copying data patches..."
    cp --recursive /app/share/rbdoom3bfg/base/* --target-directory ${rbdoom3bfg_base_dir}
    touch ${rbdoom3bfg_data_dir}/.setup
    echo "Data patches copied"
fi

# Run game engine
exec /app/bin/RBDoom3BFG "$@"
