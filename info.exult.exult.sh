#!/bin/bash

# Deploy default configuration file if needed
if [[ ! -f $XDG_CONFIG_HOME/exult.cfg ]]; then
  cp /app/share/exult/exult.default.cfg $XDG_CONFIG_HOME/exult.cfg
fi

# Redirect any attempts to read from /app/share/data to $XDG_DATA_HOME in exult.cfg
sed -i "s|\$XDG_DATA_HOME|$XDG_DATA_HOME|g" $XDG_CONFIG_HOME/exult.cfg
sed -i "s|/app/share/data|$XDG_DATA_HOME|g" $XDG_CONFIG_HOME/exult.cfg

exult -c $XDG_CONFIG_HOME/exult.cfg $@
