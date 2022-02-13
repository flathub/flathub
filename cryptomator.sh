#!/usr/bin/env sh

java --class-path "/app/Cryptomator/libs/*" \
  --module-path "/app/Cryptomator/mods" \
  -Dcryptomator.ipcPortPath="${XDG_CONFIG_HOME}"/Cryptomator/ipcPort.bin \
  -Dcryptomator.logDir="${XDG_DATA_HOME}"/Cryptomator/logs/ \
  -Dcryptomator.mountPointsDir="${XDG_DATA_HOME}"/Cryptomator/mnt/ \
  -Dcryptomator.settingsPath="${XDG_CONFIG_HOME}"/Cryptomator/settings.json \
  -Dcryptomator.ipcSocketPath="${XDG_CONFIG_HOME}"/Cryptomator/ipc.socket \
	-Xss2m \
	-Xmx512m \
	-m org.cryptomator.desktop/org.cryptomator.launcher.Cryptomator
