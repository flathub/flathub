#!/bin/bash -e
# Credits to github/nickavem for the script, Doomsdayrs for edits

ln -sfn ~/.var/app/com.atlauncher.ATLauncher/cache/ ~/.var/app/com.atlauncher.ATLauncher/data/cache
ln -sfn ~/.var/app/com.atlauncher.ATLauncher/config ~/.var/app/com.atlauncher.ATLauncher/data/configs

DIR=${CUSTOM_DIR:-".var/app/com.atlauncher.ATLauncher/data/"}

java -jar /app/bin/ATLauncher.jar --working-dir=$DIR --no-launcher-update
