#!/usr/bin/bash

bsdtar -Oxf writersolo.deb 'data.tar.xz'|bsdtar -xf - --exclude='usr'

rm writersolo.deb



#DONE!
