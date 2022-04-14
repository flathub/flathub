#!/bin/bash

if [ -d ~/retrodeck/roms ] && [ -d ~/retrodeck/roms ]
then # found both internal and sd folders
    kdialog --title "RetroDECK" --warning "I found a roms folder both in internal and SD Card, in order to make this tool useful you should remove one of the two."
    exit 0
fi

if [ -d ~/retrodeck/roms ] && [ ! -d /run/media/mmcblk0p1/retrodeck/roms ] 
then # found internal folder and not the external
    roms_path=~/retrodeck
    new_roms_path=/run/media/mmcblk0p1/retrodeck
fi

if [ ! -d ~/retrodeck/roms ] && [ -d /run/media/mmcblk0p1/retrodeck/roms ] 
then # found external folder and not the internal
    roms_path=/run/media/mmcblk0p1/retrodeck
    new_roms_path=~/retrodeck
fi

mkdir -p $new_roms_path
mv -f $roms_path/roms $new_roms_path/roms
rm -f /var/config/emulationstation/ROMs
ln -s $new_roms_path/roms /var/config/emulationstation/ROMs
rm -f $roms_path/roms
