#!/bin/bash

# source: https://github.com/flathub/org.flatpak.Builder/blob/master/fusermount-wrapper.sh

if [ -z "$_FUSE_COMMFD" ]; then
    FD_ARGS=
else
    FD_ARGS="--env=_FUSE_COMMFD=${_FUSE_COMMFD}"
    [ "$_FUSE_COMMFD" -gt 2 ] && FD_ARGS+=" --forward-fd=${_FUSE_COMMFD}"
fi

exec flatpak-spawn --host $FD_ARGS fusermount3 "$@"