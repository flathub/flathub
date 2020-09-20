#!/bin/sh

PARAM=${1+"$@"}

export WINEDEBUG=-all

/app/extra/Fightcade/emulator/fcade ${PARAM} 2>&1 &
