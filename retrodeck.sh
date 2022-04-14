#!/bin/bash

# Steam Deck SD path: /run/media/mmcblk0p1

is_mounted() {
    mount | awk -v DIR="$1" '{if ($3 == DIR) { exit 0}} ENDFILE{exit -1}'
}

# if we got the .lock file it means that it's not a first run
if [ ! -f ~/retrodeck/.lock ]
then
	kdialog --title "RetroDECK" --yes-label "Yes" --no-label "Quit" --yesno "Welcome to the first configuration of RetroDECK.\n\nBefore starting, are you in Desktop Mode?\nIf not the program will quit as the first setup MUST be done in Desktop Mode."
	if [ $? == 1 ] #quit
    then
		exit 0
	fi
    kdialog --title "RetroDECK" --yes-label "Internal" --no-label "SD Card" --yesno "Where do you want your roms folder to be located?"
    if [ $? == 0 ] #yes - Internal
    then
        roms_folder=~/retrodeck/roms
    else #no - SD Card
        if is_mounted "/run/media/mmcblk0p1"
        then
            roms_folder=/run/media/mmcblk0p1/retrodeck/roms
        else
            kdialog --title "RetroDECK" --error "SD Card is not readable, please check if it inserted or mounted correctly and run RetroDECK again."
            exit 0
        fi
    fi

    mkdir -p $roms_folder

    # initializing ES-DE

    mkdir -p /var/config/retrodeck/tools

    # Cleaning
    rm -rf /var/config/emulationstation/
    rm /var/config/retrodeck/tools/*

    kdialog --title "RetroDECK" --msgbox "EmulationStation will now initialize the system, please don't edit the roms location, just select:\n\nCREATE DIRECTORIES, YES, QUIT\n\nRetroDECK will manage the rest."

    mkdir -p /var/config/emulationstation/

    emulationstation --home /var/config/emulationstation

	kdialog --title "RetroDECK" --msgbox "RetroDECK will now install the needed files, please wait one minute, another message will notify when the process will be finished.\n\nPress OK to continue."

    mv -f /var/config/emulationstation/ROMs /var/config/emulationstation/ROMs.bak
    ln -s $roms_folder /var/config/emulationstation/ROMs
    mv -f /var/config/emulationstation/ROMs.bak/* $roms_folder/

    # XMLSTARLET HERE
    cp /app/retrodeck/es_settings.xml /var/config/emulationstation/.emulationstation/es_settings.xml

    mkdir -p ~/retrodeck/saves
    mkdir -p ~/retrodeck/states
    mkdir -p ~/retrodeck/screenshots
    mkdir -p ~/retrodeck/bios
    mkdir -p /var/config/retroarch/

    cp -r /app/retrodeck/tools/* /var/config/retrodeck/tools/

    rm -rf /var/config/retroarch/system
    ln -s ~/retrodeck/bios /var/config/retroarch/system

    cp /app/retrodeck/retrodeck-retroarch.cfg /var/config/retroarch/retroarch.cfg

    mkdir -p /var/config/emulationstation/.emulationstation/custom_systems/tools/
    cp /app/retrodeck/tools-gamelist.xml /var/config/emulationstation/.emulationstation/custom_systems/tools/gamelist.xml

    mkdir -p /var/config/retroarch/cores/
    cp /app/share/libretro/cores/* /var/config/retroarch/cores/

    touch ~/retrodeck/.lock

    kdialog --title "RetroDECK" --msgbox "Initialization completed.\nplease put your roms in:\n\n$roms_folder\n\nand your bioses in\n\n~/retrodeck/bios\n\nThen start the program again.\nIf you wish to change the roms location, you may use the tool located the tools section of RetroDECK.\n\nIt's suggested to add RetroDECK to your Steam Library for a quick access."
else
    emulationstation --home /var/config/emulationstation
fi
