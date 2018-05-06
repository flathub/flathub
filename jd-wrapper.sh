#!/bin/bash

JDURL="http://installer.jdownloader.org/JDownloader.jar"
JDDIR=$XDG_DATA_HOME/jdownloader
JDJAR=$JDDIR/JDownloader.jar

if [ ! -f $JDJAR ]; then
    mkdir -p $JDDIR
    curl $JDURL -o $JDJAR --stderr - | zenity --progress --text="Downloading JDownloader.jar" --pulsate --no-cancel --auto-close
fi

java -jar $JDJAR
