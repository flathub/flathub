#!/bin/bash
if [[ " $@ " =~ " -open-launcher " ]]
then
   mkdir -p "$HOME/.doombfa/base/lib"
   export LD_LIBRARY_PATH="$HOME/.doombfa/base/lib"
   mono /app/bin/CDL.exe $*
else
   mkdir -p "$HOME/.doombfa/base/lib"
   export LD_LIBRARY_PATH="$HOME/.doombfa/base/lib"
   (cd "$HOME/.doombfa/base/lib" && exec /app/bin/DoomBFA $*)
fi
