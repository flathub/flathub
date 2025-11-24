#!/bin/sh

# Modify /etc/hosts to block logging servers
# This is possible because /etc is a writable tmpfs in flatpak
if readlink /etc/hosts > /dev/null; then
    # /etc/hosts is a symlink by default, if it is, copy the original and modify it
    # Otherwise, we already modified it
    original=$(readlink /etc/hosts)
    rm /etc/hosts
    cp $original /etc/hosts
    cat <<EOF >> /etc/hosts
# Global
# Honkai Impact 3rd logging servers
0.0.0.0 log-upload-os.hoyoverse.com
0.0.0.0 sg-public-data-api.hoyoverse.com
0.0.0.0 dump.gamesafe.qq.com
EOF
fi

export PATH=$PATH:/usr/lib/extensions/vulkan/gamescope/bin

exec honkers-launcher "$@"
