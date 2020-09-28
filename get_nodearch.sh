#!/bin/bash

architecture=""
case $(uname -m) in
    x86_64) architecture="x64" ;;
    armv7l) architecture="armv7l" ;;
    aarch64*) architecture="arm64" ;;
    armv8*) architecture="arm64" ;;
esac
mkdir -p third_party/node/linux/node-linux-$architecture/bin/
abs_path=$(readlink -f ../../node-*-linux-$architecture/bin/node)
ln -s $abs_path third_party/node/linux/node-linux-$architecture/bin/
