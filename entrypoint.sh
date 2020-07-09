#!/usr/bin/env sh

set -o errexit

PATH="${PATH}:/app/bin:/app/jre/bin:/usr/bin"
export PATH

java --class-path "/app/Cryptomator/*" \
  -Dcryptomator.ipcPortPath="${XDG_CONFIG_HOME}/Cryptomator/ipcPort.bin" \
  -Dcryptomator.logDir="${XDG_DATA_HOME}/Cryptomator/logs" \
  -Dcryptomator.mountPointsDir="${XDG_DATA_HOME}/Cryptomator/mnt" \
  -Dcryptomator.settingsPath="${XDG_CONFIG_HOME}/Cryptomator/settings.json" \
  -Xmx512m \
  -Xss20m \
  org.cryptomator.launcher.Cryptomator
