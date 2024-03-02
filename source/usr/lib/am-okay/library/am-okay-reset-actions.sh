#!/usr/bin/bash

# Remove the directory --> "$HOME/.local/share/am-okay" to reset all actions 
# done by <<am-okay>>. This script is linked to the script <<am-okay-boot.service>> 
# that is in the directory "/etc/systemd/system/am-okay-boot.service"
# or in the directory of the project <<am-okay>> that is on the github site with the path
# "Projects/am-okay/etc/systemd/system/am-okay-boot.service" [ve-quantic repository]

if [[ -e "$HOME/.local/share/am-okay" ]]
then
    rm -r "$HOME/.local/share/am-okay" 2> /dev/null 
fi

