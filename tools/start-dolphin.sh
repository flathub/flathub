#!/bin/bash

zenity --title "RetroDECK" --question --no-wrap --window-icon="/app/share/icons/hicolor/scalable/apps/net.retrodeck.retrodeck.svg" --text="Doing some changes in the emulator's configuration may create serious issues,\nplease continue only if you know what you're doing.\n\nDo you want to continue?"
if [ $? == 0 ]
then
    dolphin-emu
fi