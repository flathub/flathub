#!/usr/bin/bash

self_dir="$(dirname $(realpath $0))"
if [[ -d "$self_dir/build" ]]; then
  export ZYPAK_BIN="$self_dir/build"
  export ZYPAK_LIB="$self_dir/build"
else
  export ZYPAK_BIN="$self_dir"
  export ZYPAK_LIB="$self_dir/../lib"
fi

preload="$ZYPAK_LIB/libzypak-preload.so"

if [[ "$1" == "-d" ]]; then
  shift
  export ZYPAK_DEBUG=1
fi

if [[ "$1" == "-s" ]]; then
  shift
  exec strace -E "LD_PRELOAD=$preload" -E PATH="$ZYPAK_BIN:$PATH" "$@"
fi

LD_PRELOAD="$preload" PATH="$ZYPAK_BIN:$PATH" exec "$@"
