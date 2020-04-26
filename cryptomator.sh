#!/usr/bin/env sh

java --class-path "/app/Cryptomator/libs/*" \
  -Dcryptomator.ipcPortPath="${XDG_CONFIG_HOME}"/Cryptomator/ipcPort.bin \
  -Dcryptomator.logDir="${XDG_DATA_HOME}"/Cryptomator/logs/ \
  -Dcryptomator.mountPointsDir="${XDG_DATA_HOME}"/Cryptomator/mnt/ \
  -Dcryptomator.settingsPath="${XDG_CONFIG_HOME}"/Cryptomator/settings.json \
  -Xmx512m \
  -Xss20m \
  org.cryptomator.launcher.Cryptomator