#!/bin/sh
set -eu

bsdtar -Oxf inkdrop.deb 'data.tar*' |
  bsdtar -xf - --strip-components=3 ./opt/Inkdrop

rm inkdrop.deb
