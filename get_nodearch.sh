#!/bin/bash

architecture=""
case $(uname -m) in
    x86_64) architecture="x64" ;;
    armv7l) architecture="armv7l" ;;
    aarch64*) architecture="arm64" ;;
    armv8*) architecture="arm64" ;;
esac
printf %s $architecture
