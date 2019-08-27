#!/usr/bin/bash

exec bitcoin-qt -datadir="${XDG_DATA_HOME}" "$@"
