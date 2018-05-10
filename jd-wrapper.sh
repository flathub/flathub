#!/bin/bash

JDDIR=$XDG_DATA_HOME/jdownloader
JDSETUP=$XDG_CACHE_HOME/JD2Setup.sh

if [ ! -f $JDDIR/JDownloader.jar ]; then
    install -Dm755 /app/extra/JD2Setup.sh $JDSETUP
    $JDSETUP -q -dir $JDDIR/tmp | zenity --progress --text="Installing JDownloader" --pulsate --no-cancel --auto-close
    mv $JDDIR/tmp/JDownloader.jar $JDDIR
    rm -rf $JDSETUP $JDDIR/tmp
    zenity --info --text "Download directory: $HOME/JDownloader" --no-wrap --title "JDownloader"
fi

java -jar $JDDIR/JDownloader.jar
