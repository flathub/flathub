#!/bin/bash

set -e
shopt -s nullglob

if [ ! -e /etc/shells ] && [ -e /var/run/host/etc/shells ]; then
  ln -s /var/run/host/etc/shells /etc/shells
fi

exec /app/bin/zypak-wrapper.sh /app/extra/BlueMail/bluemail "$@"
