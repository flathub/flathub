#!/usr/bin/bash

bsdtar -Oxf writersolo.deb 'data.tar.xz'|bsdtar -xvf - --exclude='usr'

rm writersolo.deb



#DONE!
