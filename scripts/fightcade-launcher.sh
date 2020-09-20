#!/bin/sh

DATADIR=/var/data

# Silently create/update Wine prefix
WINEDEBUG=-all DISPLAY=:invalid wineboot -u

# Create config files
mkdir -p ${DATADIR}/fc2json

# Log file Fightcade expects to be able to write to
mkdir -p /var/data/logs
touch ${DATADIR}/logs/fcade-errors.log
touch ${DATADIR}/logs/fcade.log
touch ${DATADIR}/logs/fcade.log.1
touch ${DATADIR}/logs/fcade.log.2
touch ${DATADIR}/logs/fcade.log.3

# Create persistent ROM folders if they don't exist
mkdir -p ~/ROMs/fbneo
mkdir -p ~/ROMs/ggpofba
mkdir -p ~/ROMs/snes9x

# Emulator config directory
mkdir -p ~/config
cp -n /app/extra/Fightcade/emulator/fbneo/config/fcadefbneo.default.ini ~/config/fcadefbneo.ini

# Boot Fightcade frontend
/app/bin/zypak-wrapper /app/extra/Fightcade/fc2-electron/fc2-electron
