export SHELL="flatpak-spawn --host --env=TERM=xterm-256color $(flatpak-spawn --host getent passwd $USER | awk -F : '{ print $7 }')"
/app/bin/foot
