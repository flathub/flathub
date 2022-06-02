#!/bin/bash

if [ -d ~/retrodeck/roms/pico-8 ]; then
    pico_folder=~/retrodeck/roms/pico-8
elif [ -d /run/media/mmcblk0p1/retrodeck/roms/pico-8 ]; then
    pico_folder=/run/media/mmcblk0p1/retrodeck/roms/pico-8
fi

echo $pico_folder > ~/retrodeck/.logs/retrodeck.log
~/retrodeck/bios/pico-8/pico8 -desktop ~/retrodeck/screenshots -windowed 0 -home ~/retrodeck/bios/pico-8 -root_path $pico_folder -splore >> ~/retrodeck/.logs/retrodeck.log
