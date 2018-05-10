#!/bin/bash

JDDIR=$XDG_DATA_HOME/jdownloader
JDSETUP=$XDG_CACHE_HOME/JD2Setup.sh

if [ ! -f $JDDIR/JDownloader.jar ]; then
    install -Dm755 /app/extra/JD2Setup.sh $JDSETUP
    $JDSETUP -q -dir $JDDIR | zenity --progress --text="Installing JDownloader" --pulsate --no-cancel --auto-close
    rm $JDSETUP
    zenity --info --text "Download directory: $HOME/JDownloader" --no-wrap --title "JDownloader"
fi

$JDDIR/JDownloader2
