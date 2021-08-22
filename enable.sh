#!/usr/bin/bash

if [ "X$FLATPAK_ACTION_INJECT" != "X" ]; then
    if [ -f ~/$FLATPAK_ACTION_INJECT ]; then
        . ~/$FLATPAK_ACTION_INJECT
    fi
elif [ -f ~/.action-inject ]; then
    . ~/.action-inject
fi
