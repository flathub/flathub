export SHELL="host-spawn $(flatpak-spawn --host getent passwd $USER | awk -F : '{ print $7 }')"
/app/bin/foot --term=xterm-256color
