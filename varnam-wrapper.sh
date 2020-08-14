#!/usr/bin/env sh

export VARNAM_SUGGESTIONS_DIR=~/.local/share/varnam/suggestions/

if [ ! -d $VARNAM_SUGGESTIONS_DIR ]; then
    mkdir -p $VARNAM_SUGGESTIONS_DIR
    cp -r /app/share/com.varnamproject.Varnam/suggestions/* $VARNAM_SUGGESTIONS_DIR
fi

if [ \"$1\" = \"varnamc\" ]; then
    shift;
    /app/bin/varnamc "$@"
else
    /app/bin/varnam "$@"
fi