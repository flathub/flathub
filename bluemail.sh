#!/bin/bash

set -e
shopt -s nullglob

if [ ! -e /etc/shells ] && [ -e /var/run/host/etc/shells ]; then
  ln -s /var/run/host/etc/shells /etc/shells
fi

# --no-sandbox is needed until BlueMail updates their base electron version to a version that is
# compatible with Freedesktop 22.08
exec /app/bin/zypak-wrapper.sh /app/extra/BlueMail/bluemail --no-sandbox "$@"
