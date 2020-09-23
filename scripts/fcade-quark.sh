#!/bin/sh

PARAM=${1+"$@"}

export WINEDEBUG=-all

/app/fightcade/Fightcade/emulator/fcade ${PARAM} 2>&1 &
